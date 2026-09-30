#!/usr/bin/env python3
"""Generate `src/bytecode/opcodes.h` from an engine's gdscript_function.h.

GDScriptFunction::code stores the raw opcode int, so the plugin's opcode enum has
to agree with the engine's enum order exactly. This reads the order straight out
of the engine header rather than restating it by hand.
"""
import argparse
import re
import sys

ap = argparse.ArgumentParser()
ap.add_argument("--godot", required=True, help="engine source root")
ap.add_argument("-o", "--out", default="src/bytecode/opcodes.h")
a = ap.parse_args()

path = a.godot.rstrip("/") + "/modules/gdscript/gdscript_function.h"
src = open(path).read()
m = re.search(r"enum Opcode \{(.*?)\n\t\};", src, re.S)
if not m:
    sys.exit("could not locate `enum Opcode` in " + path)

ops = []
for line in m.group(1).splitlines():
    line = line.split("//")[0].strip().rstrip(",")
    if not line:
        continue
    for tok in line.split(","):
        tok = tok.strip()
        if tok.startswith("OPCODE_"):
            ops.append(tok)

if not ops or ops[0] != "OPCODE_OPERATOR" or ops[-1] != "OPCODE_END":
    sys.exit("unexpected opcode enum shape: %r .. %r" % (ops[:1], ops[-1:]))

L = []
L.append("// GENERATED FILE -- do not edit by hand.")
L.append("// Regenerate with: python3 tools/gen_opcodes.py --godot <engine-source>")
L.append("//")
L.append("// GDScript VM opcodes, in engine enum order. The numeric values are the")
L.append("// contract: GDScriptFunction::code stores the raw opcode int, so this list")
L.append("// must match modules/gdscript/gdscript_function.h exactly.")
L.append("")
L.append("#pragma once")
L.append("")
L.append("namespace godot {")
L.append("namespace gdscript_disassemble {")
L.append("")
L.append("enum Opcode {")
for i, o in enumerate(ops):
    L.append("\t%s = %d," % (o, i))
L.append("};")
L.append("")
L.append("inline constexpr int kOpcodeCount = %d;" % len(ops))
L.append("")
L.append("// Human-readable opcode name, without the OPCODE_ prefix.")
L.append("inline const char *opcode_name(int p_opcode) {")
L.append("\tswitch (p_opcode) {")
for o in ops:
    L.append('\t\tcase %s: return "%s";' % (o, o[len("OPCODE_"):]))
L.append('\t\tdefault: return "<invalid>";')
L.append("\t}")
L.append("}")
L.append("")
L.append("} // namespace gdscript_disassemble")
L.append("} // namespace godot")
L.append("")
open(a.out, "w").write("\n".join(L))
print("wrote %s: %d opcodes" % (a.out, len(ops)))
