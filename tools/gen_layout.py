#!/usr/bin/env python3
"""Generate `src/bytecode/engine_layout.h` for a Godot source tree.

The GDExtension API does not expose GDScript's compiled bytecode, so the plugin
reads the live `GDScript` / `GDScriptFunction` objects directly. That requires
the exact byte offsets of their members *for the engine build that will load
this extension*, which in turn depends on the engine version and on which
`#ifdef`s (TOOLS_ENABLED, DEBUG_ENABLED, ...) were active.

Rather than hand-replicating the class layouts (which silently produces wrong
offsets the moment a container changes size), this tool asks the engine's own
headers, compiled with the engine's own flags, what the offsets are.

How it works: the probe declares `P<offsetof(T, m)>` for an undefined template
`P`. Clang diagnoses the implicit instantiation and prints the value in the
message. `-fsyntax-only` means nothing is linked and nothing is executed -- the
numbers come straight out of the compiler's front end, and the probe needs no
main(), no engine library, and no running engine.

Usage:
    python3 tools/gen_layout.py --godot /path/to/godot                 > src/bytecode/engine_layout.h
    python3 tools/gen_layout.py --godot /path/to/godot --check         # verify an existing header

Requires the target tree to have been configured for building (compile_commands.json),
or --flags-from to point at another tree's compile_commands.json that used the
same target/platform/arch.
"""

import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

# ---------------------------------------------------------------------------
# Probe source
# ---------------------------------------------------------------------------

# `#define private public` is what lets offsetof() see the private members. It
# is applied only around the two engine headers, and std headers are pulled in
# first so the redefinition cannot corrupt them. This is a build-time-only probe
# that never ships, so the usual objections to the trick do not apply.
PROBE_HEAD = r"""
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <initializer_list>

#define private public
#define protected public
#include "modules/gdscript/gdscript.h"
#include "modules/gdscript/gdscript_function.h"
#include "core/object/class_db.h"
#undef private
#undef protected

template <size_t N> struct P;
#define CAT(a, b) a##b
#define XCAT(a, b) CAT(a, b)
#define SZ(...) P<sizeof(__VA_ARGS__)> XCAT(s_, __LINE__)
#define OFF(T, M) P<offsetof(T, M)> XCAT(o_, __LINE__)
"""

# name -> probe expression. Order is preserved in the generated header.
PROBES = [
    # ("SIZE", "Object"), ...
    ("SIZE", "Object"), ("SIZE", "RefCounted"), ("SIZE", "Resource"),
    ("SIZE", "Script"), ("SIZE", "GDScript"), ("SIZE", "GDScriptFunction"),

    ("OFFSET", "GDScript", "member_functions"),
    ("OFFSET", "GDScript", "constants"),
    ("OFFSET", "GDScript", "subclasses"),
    ("OFFSET", "GDScript", "member_indices"),
    ("OFFSET", "GDScript", "members"),
    ("OFFSET", "GDScript", "static_variables"),
    ("OFFSET", "GDScript", "static_variables_indices"),
    ("OFFSET", "GDScript", "lambda_info"),
    ("OFFSET", "GDScript", "implicit_initializer"),
    ("OFFSET", "GDScript", "implicit_ready"),
    ("OFFSET", "GDScript", "static_initializer"),
    ("OFFSET", "GDScript", "valid"),
    ("OFFSET", "GDScript", "base"),
    # Names of a script constant. The engine's own disassembler resolves these
    # through Ref<Script> accessors, which we cannot call without a wrapper --
    # so they are read as fields instead.
    ("OFFSET", "GDScript", "local_name"),
    ("OFFSET", "GDScript", "fully_qualified_name"),
    ("OFFSET", "GDScriptNativeClass", "name"),
    ("OFFSET", "GDScript", "native"),

    ("OFFSET", "GDScriptFunction", "name"),
    ("OFFSET", "GDScriptFunction", "method_info"),
    ("OFFSET", "GDScriptFunction", "_script"),
    ("OFFSET", "GDScriptFunction", "_static"),
    ("OFFSET", "GDScriptFunction", "_initial_line"),
    ("OFFSET", "GDScriptFunction", "_argument_count"),
    ("OFFSET", "GDScriptFunction", "_vararg_index"),
    ("OFFSET", "GDScriptFunction", "_stack_size"),
    ("OFFSET", "GDScriptFunction", "code"),
    ("OFFSET", "GDScriptFunction", "constants"),
    ("OFFSET", "GDScriptFunction", "global_names"),
    ("OFFSET", "GDScriptFunction", "methods"),
    ("OFFSET", "GDScriptFunction", "lambdas"),
    ("OFFSET", "GDScriptFunction", "_code_size"),
    ("OFFSET", "GDScriptFunction", "_constant_count"),
    ("OFFSET", "GDScriptFunction", "_global_names_count"),
    ("OFFSET", "GDScriptFunction", "_code_ptr"),
    ("OFFSET", "GDScriptFunction", "_constants_ptr"),
    ("OFFSET", "GDScriptFunction", "_global_names_ptr"),
    ("OFFSET", "GDScriptFunction", "_methods_ptr"),
    ("OFFSET", "GDScriptFunction", "_lambdas_ptr"),

    # DEBUG_ENABLED-only tables that map an operand index to the name the engine
    # itself would print (operator, setter, constructor, ...). Reading these is
    # how the listing gets real names without restating any enum by hand.
    ("OFFSET", "GDScriptFunction", "operator_names"),
    ("OFFSET", "GDScriptFunction", "setter_names"),
    ("OFFSET", "GDScriptFunction", "getter_names"),
    ("OFFSET", "GDScriptFunction", "builtin_methods_names"),
    ("OFFSET", "GDScriptFunction", "constructors_names"),
    ("OFFSET", "GDScriptFunction", "utilities_names"),
    ("OFFSET", "GDScriptFunction", "gds_utilities_names"),

    ("OFFSET", "MethodBind", "name"),
    ("OFFSET", "MethodBind", "argument_count"),

    # Value types of the maps we iterate. We never read their fields, but the
    # mirror has to agree on their size for the container's element stride to
    # come out right.
    ("SIZE", "GDScript::LambdaInfo"),
    ("SIZE", "GDScript::MemberInfo"),
    ("OFFSET", "GDScript::MemberInfo", "index"),

    # Sanity anchors: these are re-measured by the runtime verifier, so they are
    # emitted too. A mismatch at runtime means the layout header does not
    # describe the engine that loaded us.
    ("SIZE", "Variant"), ("SIZE", "StringName"), ("SIZE", "String"),
    ("SIZE", "Vector<int>"), ("SIZE", "HashMap<int, int>"), ("SIZE", "List<int>"),
]


def sanitize(ident):
    """`GDScript::MemberInfo` and `HashMap<int, int>` both have to become valid
    C++ identifiers. Leading underscores are kept: they are part of the member
    name (`_code_ptr`) and dropping them would create collisions."""
    return re.sub(r"[^A-Za-z0-9]+", "_", ident)


def const_name(kind, *rest):
    if kind == "SIZE":
        # `<`/`>` at the ends of a template name produce edge underscores.
        return "kSize_" + sanitize(rest[0]).strip("_")
    return "kOff_" + "_".join(sanitize(r) for r in rest)


def run_probe(probe_src, cwd, args):
    """Compile the probe and scrape the values clang prints in its diagnostics."""
    with tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False) as f:
        f.write(probe_src)
        path = f.name
    try:
        proc = subprocess.run(
            args + ["-fsyntax-only", "-fno-color-diagnostics", "-ferror-limit=0", path],
            cwd=cwd, capture_output=True, text=True,
        )
    finally:
        os.unlink(path)

    out = proc.stdout + proc.stderr
    if "fatal error" in out:
        for line in out.splitlines():
            if "fatal error" in line:
                sys.exit("probe failed to compile: " + line.strip())

    tag = os.path.basename(path).replace(".", r"\.")
    values = {}
    for m in re.finditer(tag + r":(\d+):\d+: error: implicit instantiation of undefined template 'P<(\d+)>'", out):
        values[int(m.group(1))] = int(m.group(2))
    return values, path


def extract_flags(godot_dir, flags_from=None):
    """Pull the engine's own compile flags for a GDScript translation unit."""
    src_dir = flags_from or godot_dir
    cc_path = os.path.join(src_dir, "compile_commands.json")
    if not os.path.exists(cc_path):
        sys.exit(
            f"{cc_path} not found.\n"
            "Build/configure the engine first (scons will write it), or pass --flags-from."
        )
    with open(cc_path) as f:
        db = json.load(f)

    entry = None
    for e in db:
        f_ = e.get("file", "")
        if f_.endswith("modules/gdscript/gdscript.cpp") or f_.endswith("gdscript.cpp") and "gdscript" in f_:
            entry = e
            break
    if entry is None:
        sys.exit("no modules/gdscript/gdscript.cpp entry in the compile database")

    cmd = shlex.split(entry["command"])
    out, skip = [], False
    for tok in cmd:
        if skip:
            skip = False
            continue
        if tok == "-o":
            skip = True
            continue
        if tok in ("-c", "-MF") or tok.endswith(".cpp") or tok.startswith("-M"):
            continue
        out.append(tok)
    workdir = entry.get("directory", src_dir)
    return workdir, out


def collect(cwd, args):
    # Tag each probe line with a marker comment and recover the line number from
    # the generated text, so the mapping does not depend on PROBE_HEAD's length.
    body, markers = [], {}
    for i, p in enumerate(PROBES):
        expr = f"SZ({p[1]});" if p[0] == "SIZE" else f"OFF({p[1]}, {p[2]});"
        body.append(f"{expr} //@probe{i}")
        markers[f"//@probe{i}"] = const_name(*p)
    src = PROBE_HEAD + "\n".join(body) + "\n"

    line_to_name = {}
    for n, line in enumerate(src.splitlines(), start=1):
        for marker, name in markers.items():
            if line.rstrip().endswith(marker):
                line_to_name[n] = name

    values, _ = run_probe(src, cwd, args)

    result = {}
    for n, name in line_to_name.items():
        if n not in values:
            sys.exit(f"probe produced no value for {name} (compiler rejected the expression?)")
        result[name] = values[n]

    missing = [const_name(*p) for p in PROBES if const_name(*p) not in result]
    if missing:
        sys.exit(f"probe lines not located for: {missing}")
    return result


def detect_version(godot_dir):
    vp = os.path.join(godot_dir, "version.py")
    info = {"major": "?", "minor": "?", "patch": "?", "status": "?"}
    if os.path.exists(vp):
        with open(vp) as f:
            body = f.read()
        for key in info:
            # version.py mixes quoted strings (status) with bare numbers (major).
            m = re.search(rf'^{key}\s*=\s*"?([^"\n]*?)"?\s*$', body, re.M)
            if m:
                info[key] = m.group(1).strip()
    return info


HEADER = """\
// GENERATED FILE -- do not edit by hand.
// Regenerate with: python3 tools/gen_layout.py --godot <engine-source>
//
// Member offsets of the engine's GDScript / GDScriptFunction, as measured from
// the engine headers themselves with the engine's own compile flags.
//
// Engine version : {major}.{minor}.{patch}-{status}
// Build flags    : {flagsum}
// Layout id      : {layout_id}
//
// Offsets depend on the engine version AND on which #ifdefs (TOOLS_ENABLED,
// DEBUG_ENABLED, ...) were active. `verify_layout()` cross-checks the entries
// marked [anchor] against the running engine before any bytecode is read, so a
// mismatched build degrades to a clear message instead of reading garbage.

#pragma once

#include <cstddef>

namespace godot {{
namespace gdscript_disassemble {{

// Identifies the engine build this table was generated from.
inline constexpr const char *kLayoutEngineVersion = "{major}.{minor}.{patch}";
inline constexpr const char *kLayoutBuildFlags = "{flagsum}";
inline constexpr unsigned long long kLayoutId = {layout_id}ULL;

struct EngineLayout {{
{members}
}};

// The table below is the active layout.
inline constexpr EngineLayout kLayout = {{
{initializers}
}};

}} // namespace gdscript_disassemble
}} // namespace godot
"""


def emit(values, info, flagsum):
    """Render the header, marking sizes/offsets and grouping the anchors."""
    lines = []
    order = []
    for p in PROBES:
        cn = const_name(*p)
        if cn not in values:
            continue
        comment = ""
        if p[0] == "SIZE":
            comment = "  // sizeof({})".format(p[1])
            if p[1] in ("Variant", "StringName", "String", "Vector<int>", "HashMap<int, int>", "List<int>"):
                comment += "  [anchor]"
        else:
            comment = f"  // offsetof({p[1]}, {p[2]})"
        lines.append(f"\tsize_t {cn} = {values[cn]};{comment}")
        order.append(cn)

    # stable layout id from the measured values + version, so a regenerated
    # header that produces identical numbers keeps an identical id
    payload = repr(sorted(values.items())) + str(info)
    layout_id = hashlib.sha256(payload.encode()).hexdigest()[:16]
    layout_id_int = int(layout_id, 16)

    members = "\n".join(lines)
    initializers = "\n".join(f"\t.{cn} = {values[cn]}," for cn in order)

    return HEADER.format(
        members=members, initializers=initializers,
        layout_id=layout_id_int, flagsum=flagsum, **info,
    )


def summarize_flags(args):
    interesting = sorted({
        a for a in args
        if a.startswith("-D") and any(k in a for k in
                                      ("TOOLS_ENABLED", "DEBUG_ENABLED", "NDEBUG", "DEV_ENABLED"))
    })
    return " ".join(interesting) if interesting else "(none)"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot", required=True, help="engine source root")
    ap.add_argument("--flags-from", default=None,
                    help="tree whose compile_commands.json holds the flags (defaults to --godot)")
    ap.add_argument("--check", action="store_true",
                    help="compare against the existing header instead of printing a new one")
    ap.add_argument("-o", "--out", default="src/bytecode/engine_layout.h")
    args_ns = ap.parse_args()

    godot_dir = os.path.abspath(args_ns.godot)
    cwd, flags = extract_flags(godot_dir, args_ns.flags_from)
    flagsum = summarize_flags(flags)
    values = collect(cwd, flags)
    info = detect_version(godot_dir)
    text = emit(values, info, flagsum)

    if args_ns.check:
        if not os.path.exists(args_ns.out):
            sys.exit(f"{args_ns.out} does not exist; nothing to check")
        with open(args_ns.out) as f:
            existing = f.read()
        want = [l for l in existing.splitlines() if " = " in l or ".k" in l]
        got = [l for l in text.splitlines() if " = " in l or ".k" in l]
        if want == got:
            print(f"OK: {args_ns.out} matches {godot_dir} ({info['major']}.{info['minor']}.{info['patch']})")
            return
        print("MISMATCH between the engine tree and the generated header:")
        import difflib
        for line in difflib.unified_diff(want, got, "header", "engine", lineterm="", n=1):
            print(line)
        sys.exit(1)

    with open(args_ns.out, "w") as f:
        f.write(text)
    print(f"wrote {args_ns.out}: {len(values)} constants from "
          f"{info['major']}.{info['minor']}.{info['patch']}-{info['status']} [{flagsum}]")


if __name__ == "__main__":
    main()
