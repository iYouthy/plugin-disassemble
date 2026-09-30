#!/usr/bin/env python3
"""Generate `src/bytecode/operand_plan.h` from the engine's disassembler.

Variadic instructions (calls, constructors, lambdas) carry their operands as
`[op][N][args..][trailer..]`, where the trailer positions are *not* stated
anywhere -- the engine computes them from either the instruction-arg count (N)
or the real argument count (argc), which are two different numbers.

Hand-transcribing those offsets has produced three separate bugs so far (a
lambda index one slot too far, which crashed the editor; a constructor index
read off the wrong count; empty method names on calls). So they are read out of
the engine's own switch instead.

The engine's convention, once you look for it, is uniform:

    int argc  = _code_ptr[ip + 1 + instr_var_args];   -> real argument count
    DADDR(1 + argc)                                   -> destination
    DADDR(1 + i)      for i < argc                    -> the arguments
    _code_ptr[ip + K + instr_var_args]                -> trailing index operand

where `ip` has already been advanced past the opcode. This tool extracts those
four positions per opcode.

Usage:
    python3 tools/gen_operand_plan.py --godot /path/to/godot
"""

import argparse
import re
import sys

ap = argparse.ArgumentParser()
ap.add_argument("--godot", required=True, help="engine source root")
ap.add_argument("-o", "--out", default="src/bytecode/operand_plan.h")
a = ap.parse_args()

path = a.godot.rstrip("/") + "/modules/gdscript/gdscript_disassembler.cpp"
src = open(path).read()
body = src[src.index("void GDScriptFunction::disassemble("):]
body = body[:body.index("\n}\n")]

# Split into `case OPCODE_*:` chunks, then carry labels forward: several opcodes
# share one body by stacking labels, so the earlier ones would otherwise look
# like they read nothing.
marks = [(m.start(), m.group(1)) for m in re.finditer(r"^\s*case (OPCODE_[A-Z0-9_]+):", body, re.M)]
chunks, pending = {}, []
for i, (pos, name) in enumerate(marks):
    stop = marks[i + 1][0] if i + 1 < len(marks) else len(body)
    chunk = body[pos:stop]
    # A *stacked* label has no body at all -- just the label and whitespace before
    # the next case. Distinguish that from a body which simply reads no operands
    # (BREAKPOINT, END), which is a real instruction.
    if not re.sub(r"^\s*case OPCODE_[A-Z0-9_]+:\s*", "", chunk).strip():
        pending.append(name)
        continue
    for p in pending:
        chunks[p] = chunk
    pending.clear()
    chunks[name] = chunk
if pending:
    sys.exit("stacked case labels with no body: %s" % ", ".join(pending))

plans = {}
fixed = {}
for name, chunk in chunks.items():
    if "++ip" not in chunk:
        # Fixed-shape instruction: every operand is at a constant offset, so the
        # only thing worth extracting is *which* offsets the engine reads and
        # whether each is an address operand or a raw int. Getting that kind
        # wrong is a real bug class -- reading an address slot as a name index
        # silently yields a wrong value rather than an obvious failure.
        addrs, imms = [], []
        for m in re.finditer(r"DADDR\(\s*(\d+)\s*\)", chunk):
            k = int(m.group(1))
            if k not in addrs:
                addrs.append(k)
        # `_code_ptr[ip + k]` only appears for reads that are NOT addresses;
        # DADDR's expansion is not visible in the source text.
        for m in re.finditer(r"_code_ptr\[ip\s*\+\s*(\d+)\]", chunk):
            k = int(m.group(1))
            if k not in imms:
                imms.append(k)
        if addrs or imms:
            fixed[name] = (addrs, imms)
        continue

    # The count variable: `int X = _code_ptr[ip + K + instr_var_args];`
    m = re.search(r"\bint\s+(\w+)\s*=\s*_code_ptr\[ip\s*\+\s*(\d+)\s*\+\s*instr_var_args\];", chunk)
    if not m:
        continue
    count_var, count_k = m.group(1), int(m.group(2))
    # ip == ip0 + 1 in these cases, so ip + K + N is code[ip0 + 1 + K + N].
    argc_slot = 1 + count_k

    # Addresses indexed by the count, in source order. Calls have two (the
    # destination and the object being called); constructors and lambdas have
    # one. Which is which is the renderer's business -- what matters here is that
    # the *positions* come from the engine.
    # Dictionaries index the destination with a doubled stride: DADDR(1 + argc*2).
    dst_slots = []
    for m in re.finditer(r"DADDR\(\s*(\d+)\s*\+\s*" + re.escape(count_var) +
                         r"(?:\s*\*\s*(\d+))?\s*\)", chunk):
        dst_slots.append((1 + int(m.group(1)), int(m.group(2)) if m.group(2) else 1))
    dst_slot = dst_slots[0][0] if dst_slots else None
    dst_mul = dst_slots[0][1] if dst_slots else 1
    dst2_slot = dst_slots[1][0] if len(dst_slots) > 1 else None
    dst2_mul = dst_slots[1][1] if len(dst_slots) > 1 else 1

    # The engine writes the argument walk in several textual forms that all mean
    # the same thing: DADDR(1 + i), DADDR(i + 1), and for dictionaries a
    # two-slot stride: DADDR(1 + i*2 + 0) / DADDR(1 + i*2 + 1).
    arg_base = None
    arg_step = 1
    for rx in (r"DADDR\(\s*(\d+)\s*\+\s*i\s*\*\s*(\d+)\s*\+",
               r"DADDR\(\s*(\d+)\s*\+\s*i\s*\)",
               r"DADDR\(\s*i\s*\*\s*(\d+)\s*\+\s*(\d+)\s*\)",
               r"DADDR\(\s*i\s*\+\s*(\d+)\s*\)"):
        ma = re.search(rx, chunk)
        if not ma:
            continue
        g = ma.groups()
        if rx.startswith(r"DADDR\(\s*i\s*\*"):
            arg_step, arg_base = int(g[0]), 1 + int(g[1])
        elif len(g) == 2:
            arg_base, arg_step = 1 + int(g[0]), int(g[1])
        else:
            arg_base, arg_step = 1 + int(g[0]), 1
        break

    # The trailing index operand: any other `_code_ptr[ip + K + <count|N>]`.
    payload_k = None
    payload_by_count = False
    for mp in re.finditer(r"_code_ptr\[ip\s*\+\s*(\d+)\s*\+\s*(instr_var_args|" + re.escape(count_var) + r")\]", chunk):
        k = int(mp.group(1))
        if k == count_k:
            continue  # that is the count read itself
        payload_k = k
        # The trailer may be indexed by the real argument count or by the raw
        # instruction-arg count. Those are different numbers, so which one it is
        # has to be carried through to the emitted expression.
        payload_by_count = (mp.group(2) != "instr_var_args")
        break
    payload_slot = (1 + payload_k, payload_by_count) if payload_k is not None else None

    # Type descriptors for the typed container ops. The engine names its locals
    # by role ("key_script_type", "value_builtin_type", or bare "script_type"
    # for the single-element array), so the role can be read off the source.
    types = {}
    for role, suffix, reader in (
            ("script", "script_type", r"get_constant\(_code_ptr\[ip \+ argc(?:\s*\*\s*(\d+))?\s*\+\s*(\d+)\]"),
            ("builtin", "builtin_type", r"\(Variant::Type\)\s*_code_ptr\[ip \+ argc(?:\s*\*\s*(\d+))?\s*\+\s*(\d+)\]"),
            ("native", "native_type", r"get_global_name\(_code_ptr\[ip \+ argc(?:\s*\*\s*(\d+))?\s*\+\s*(\d+)\]")):
        for m in re.finditer(r"(\w*?)_?" + suffix + r"\s*=\s*" + reader, chunk):
            host = m.group(1) or "elem"
            mul = int(m.group(2)) if m.group(2) else 1
            k = int(m.group(3))
            types.setdefault(host, {})[role] = (1 + k, mul)

    plans[name] = (argc_slot, arg_base, arg_step, dst_slot, dst_mul, dst2_slot, dst2_mul, payload_slot, types)

if not plans:
    sys.exit("no variadic opcodes recognised -- did the engine refactor the disassembler?")

L = []
L.append("// GENERATED FILE -- do not edit by hand.")
L.append("// Regenerate with: python3 tools/gen_operand_plan.py --godot <engine-source>")
L.append("//")
L.append("// Where each variadic instruction keeps its argument count, arguments,")
L.append("// destination, and trailing index operand. Read out of the engine's own")
L.append("// disassembler so the plugin cannot disagree with it.")
L.append("")
L.append("#pragma once")
L.append("")
L.append("#include \"opcodes.h\"")
L.append("")
L.append("namespace godot {")
L.append("namespace gdscript_disassemble {")
L.append("")
L.append("// All slots are relative to `ip0`, the opcode's own index, and are resolved")
L.append("// against the live instruction stream because the trailing operands sit at")
L.append("// a position that depends on the argument count. A value of -1 means the")
L.append("// opcode does not have that operand.")
L.append("struct OperandPlan {")
L.append("\tint argc = -1;    // slot holding the real argument count")
L.append("\tint args = -1;      // slot of the first argument")
L.append("\tint args_step = 1;  // stride between arguments (2 for dictionaries)")
L.append("\tint dst = -1;     // first count-indexed address (usually the destination)")
L.append("\tint dst2 = -1;      // second count-indexed address, where the opcode has one")
L.append("\tint payload = -1; // slot of the trailing index operand (method, ctor, ...)")
L.append("")
L.append("\t// Element/key/value type descriptors for the typed container ops:")
L.append("\t// script constant address, Variant::Type, and native name index.")
L.append("\tint elem_script = -1, elem_builtin = -1, elem_native = -1;")
L.append("\tint key_script = -1, key_builtin = -1, key_native = -1;")
L.append("\tint value_script = -1, value_builtin = -1, value_native = -1;")
L.append("};")
L.append("")
L.append("inline OperandPlan operand_plan(int p_opcode, const int *code, int ip0) {")
L.append("\tOperandPlan plan;")
L.append("\tswitch (p_opcode) {")
for name in sorted(plans):
    argc_slot, arg_base, arg_step, dst_slot, dst_mul, dst2_slot, dst2_mul, payload_slot, types = plans[name]
    # The count itself lives at `ip + K + N`, i.e. one slot past the N operands.
    expr = ["plan.argc = ip0 + %d + code[ip0 + 1];" % argc_slot]
    if arg_base is not None:
        expr.append("plan.args = ip0 + %d;" % arg_base)
        expr.append("plan.args_step = %d;" % arg_step)
    if dst_slot is not None:
        expr.append("plan.dst = ip0 + %d + code[plan.argc] * %d;" % (dst_slot, dst_mul))
    if dst2_slot is not None:
        expr.append("plan.dst2 = ip0 + %d + code[plan.argc] * %d;" % (dst2_slot, dst2_mul))
    if payload_slot is not None:
        slot, by_count = payload_slot
        # `code[plan.argc]` is the argument count itself; `code[ip0 + 1]` is N.
        base = "code[plan.argc]" if by_count else "code[ip0 + 1]"
        expr.append("plan.payload = ip0 + %d + %s;" % (slot, base))
    for host in ("elem", "key", "value"):
        for role in ("script", "builtin", "native"):
            if host in types and role in types[host]:
                slot, mul = types[host][role]
                expr.append("plan.%s_%s = ip0 + %d + code[plan.argc] * %d;" % (host, role, slot, mul))
    L.append("\t\tcase %s: { %s } break;" % (name, " ".join(expr)))
L.append("\t\tdefault: break;")
L.append("\t}")
L.append("\treturn plan;")
L.append("}")
L.append("")
L.append("// Operands of the fixed-shape instructions, split by kind. Positions are")
L.append("// in the order the engine reads them, so the order *is* the engine's, not a")
L.append("// restatement of it.")
L.append("//")
L.append("// The kind matters as much as the position: an address operand and a raw int")
L.append("// are read through the same slot but mean different things, and reading one as")
L.append("// the other yields a plausible-looking wrong value.")
L.append("struct FixedPlan {")
L.append("\tint addr[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };")
L.append("\tint addr_count = 0;")
L.append("\tint imm[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };")
L.append("\tint imm_count = 0;")
L.append("};")
L.append("")
L.append("inline FixedPlan fixed_plan(int p_opcode, int ip0) {")
L.append("\tFixedPlan plan;")
L.append("\tswitch (p_opcode) {")
for _name in sorted(fixed):
    _addrs, _imms = fixed[_name]
    _parts = []
    for _i, _k in enumerate(_addrs[:8]):
        _parts.append("plan.addr[%d] = ip0 + %d;" % (_i, _k))
    _parts.append("plan.addr_count = %d;" % min(len(_addrs), 8))
    for _i, _k in enumerate(_imms[:8]):
        _parts.append("plan.imm[%d] = ip0 + %d;" % (_i, _k))
    _parts.append("plan.imm_count = %d;" % min(len(_imms), 8))
    L.append("\t\tcase %s: { %s } break;" % (_name, " ".join(_parts)))
L.append("\t\tdefault: break;")
L.append("\t}")
L.append("\treturn plan;")
L.append("}")
L.append("")
L.append("} // namespace gdscript_disassemble")
L.append("} // namespace godot")
L.append("")
open(a.out, "w").write("\n".join(L))

print("wrote %s: %d variadic opcodes" % (a.out, len(plans)))
for n in sorted(plans):
    pl = plans[n]
    pl_slot = pl[7]
    print("  %-42s argc_slot=+%d args=%s step=%d dst=%s dst2=%s payload=%s\n" % (n, pl[0], pl[1], pl[2], pl[3], pl[4], (("+%d+%s" % (pl_slot[0], "argc" if pl_slot[1] else "N")) if pl_slot is not None else "-")))