#!/usr/bin/env python3
"""Generate `src/bytecode/instruction_sizes.h` from the engine's disassembler.

The instruction stream has no length prefix: decoding it means knowing, for each
opcode, how many ints it occupies. Get one wrong and every subsequent line is
garbage. There is no table in the engine to copy -- the layout is hard-coded in
the big switch inside GDScriptFunction::disassemble().

So we read that switch and pull the advance out of it mechanically. Every case
ends with an `incr` update; that value *is* the instruction's length. Deriving it
here means the plugin cannot disagree with the engine about instruction length,
which is the one thing that would make the whole listing nonsense.

Usage:
    python3 tools/gen_instruction_sizes.py --godot /path/to/godot
"""

import argparse
import re
import sys

ap = argparse.ArgumentParser()
ap.add_argument("--godot", required=True, help="engine source root")
ap.add_argument("-o", "--out", default="src/bytecode/instruction_sizes.h")
a = ap.parse_args()

path = a.godot.rstrip("/") + "/modules/gdscript/gdscript_disassembler.cpp"
src = open(path).read()

# Isolate the body of GDScriptFunction::disassemble().
start = src.index("void GDScriptFunction::disassemble(")
body = src[start:]
end = body.index("\n}\n")
body = body[:end]

# Split into `case OPCODE_*:` chunks.
case_re = re.compile(r"^\s*case (OPCODE_[A-Z0-9_]+):", re.M)
marks = [(m.start(), m.group(1)) for m in case_re.finditer(body)]
if not marks:
    sys.exit("no `case OPCODE_*:` found -- did the engine refactor the disassembler?")

chunks = []
for i, (pos, name) in enumerate(marks):
    stop = marks[i + 1][0] if i + 1 < len(marks) else len(body)
    chunks.append((name, body[pos:stop]))

def advance_of(text):
    """Pull the `incr` update out of a case body and express it in terms of the
    opcode's own index (ip0) and the instruction's first slot."""
    m = re.search(r"incr\s*\+=\s*([^;]+);|incr\s*=\s*([^;]+);", text)
    if m is None:
        return None
    expr = (m.group(1) or m.group(2)).strip()

    # In variadic cases the engine does `int x = _code_ptr[++ip];` first, so `ip`
    # has already moved one slot by the time `incr` is applied.
    variadic = "++ip" in text
    expr = expr.replace("_pointer_size", "kPointerSize")

    if variadic:
        # The engine lifts operands into locals before computing `incr`. Resolve
        # those locals rather than special-casing each name: `++ip` makes
        # ip == ip0 + 1, so `_code_ptr[ip + K + instr_var_args]` is
        # code[ip0 + K + 1 + N].
        names = {}
        for m in re.finditer(r"\bint\s+(\w+)\s*=\s*_code_ptr\[\+\+ip\];", text):
            names[m.group(1)] = "N"
        for m in re.finditer(r"\bint\s+(\w+)\s*=\s*_code_ptr\[ip\s*\+\s*(\d+)\s*\+\s*instr_var_args\];", text):
            names[m.group(1)] = "code[ip0 + %d + N]" % (int(m.group(2)) + 1)
        for name, repl in names.items():
            expr = re.sub(r"\b" + name + r"\b", "(" + repl + ")", expr)
        expr = "1 + " + expr

    return expr.replace("N", "code[ip0 + 1]")


sizes = {}

# Several opcodes can share one body by stacking labels:
#     case OPCODE_CALL:
#     case OPCODE_CALL_RETURN:
#     case OPCODE_CALL_METHOD_BIND: { ... incr += ...; }
# Splitting on `case` leaves the earlier labels with empty bodies, so carry them
# forward until a body that actually advances shows up.
pending = []
for name, chunk in chunks:
    expr = advance_of(chunk)
    if expr is None:
        pending.append(name)
        continue
    for p in pending:
        sizes[p] = expr
    pending.clear()
    sizes[name] = expr
if pending:
    sys.exit("stacked case labels with no body: %s" % ", ".join(pending))

# Type-adjust ops are named only through a macro invocation.
for op in re.findall(r"DISASSEMBLE_TYPE_ADJUST\(([A-Z0-9_]+)\)", body):
    sizes.setdefault("OPCODE_TYPE_ADJUST_" + op, "2")

# The iterate ops come from DISASSEMBLE_ITERATE_TYPES(<macro>), which expands one
# macro body once per Variant type.
tm = re.search(r"#define DISASSEMBLE_ITERATE_TYPES\(m_macro\)(.*?)\n\n", src, re.S)
if tm:
    type_list = re.findall(r"m_macro\(([A-Z0-9_]+)\)", tm.group(1))
    macro_bodies = {}

    def macro_body(name):
        if name not in macro_bodies:
            # The body is a run of backslash-continued lines, so it ends at the
            # first genuinely blank line. (Terminating on `\n\t+case` would stop
            # immediately -- the macro's own first line is a `case`.)
            mm = re.search(r"#define " + name + r"\(m_type\)(.*?)(?=\n\n|\n#undef|\Z)", src, re.S)
            macro_bodies[name] = mm.group(1) if mm else ""
        return macro_bodies[name]

    # The macros are #defined *inside* the function, so their definitions would
    # otherwise match as invocations.
    invocations = [
        line for line in body.splitlines() if "DISASSEMBLE_ITERATE_TYPES(" in line and not line.lstrip().startswith("#define")
    ]
    for line in invocations:
        macro = re.search(r"DISASSEMBLE_ITERATE_TYPES\((\w+)\)", line).group(1)
        # DISASSEMBLE_ITERATE_BEGIN -> OPCODE_ITERATE_BEGIN_<TYPE>
        base = macro[len("DISASSEMBLE_"):]
        expr = advance_of(macro_body(macro))
        if expr is None:
            sys.exit("no advance in iterate macro %s" % macro)
        for t in type_list:
            sizes.setdefault("OPCODE_%s_%s" % (base, t), expr)

# Cases defined by a case-range or missing entirely: check against the enum.
header = open(a.godot.rstrip("/") + "/modules/gdscript/gdscript_function.h").read()
em = re.search(r"enum Opcode \{(.*?)\n\t\};", header, re.S)
all_ops = []
for line in em.group(1).splitlines():
    line = line.split("//")[0].strip().rstrip(",")
    if not line:
        continue
    for tok in line.split(","):
        tok = tok.strip()
        if tok.startswith("OPCODE_"):
            all_ops.append(tok)

missing = [o for o in all_ops if o not in sizes]
if missing:
    sys.exit("no length derived for: %s" % ", ".join(missing))

L = []
L.append("// GENERATED FILE -- do not edit by hand.")
L.append("// Regenerate with: python3 tools/gen_instruction_sizes.py --godot <engine-source>")
L.append("//")
L.append("// Instruction lengths, extracted from the advance (`incr`) the engine's own")
L.append("// disassembler applies to each opcode. Decoding the stream without this is")
L.append("// impossible -- instructions carry no length of their own -- and getting one")
L.append("// wrong desynchronises everything after it.")
L.append("")
L.append("#pragma once")
L.append("")
L.append("#include \"opcodes.h\"")
L.append("")
L.append("namespace godot {")
L.append("namespace gdscript_disassemble {")
L.append("")
L.append("// sizeof(Variant::ValidatedOperatorEvaluator) / sizeof(int) -- 2 on 64-bit.")
L.append("inline constexpr int kPointerSize = (int)(sizeof(void *) / sizeof(int));")
L.append("")
L.append("// Length in ints of the instruction starting at p_ip, or 0 if the opcode is")
L.append("// not one this table knows (which callers must treat as a decode failure).")
L.append("// `code` is the whole instruction stream and `ip0` the opcode's slot, matching")
L.append("// the naming the engine's own disassembler uses for the same expression.")
L.append("inline int instruction_length(int p_opcode, const int *code, int ip0) {")
L.append("\tswitch (p_opcode) {")
for op in all_ops:
    L.append("\t\tcase %s: return %s;" % (op, sizes[op]))
L.append("\t\tdefault: return 0;")
L.append("\t}")
L.append("}")
L.append("")
L.append("} // namespace gdscript_disassemble")
L.append("} // namespace godot")
L.append("")
open(a.out, "w").write("\n".join(L))
print("wrote %s: %d opcodes" % (a.out, len(all_ops)))
