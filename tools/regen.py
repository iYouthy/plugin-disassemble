#!/usr/bin/env python3
"""Regenerate every layout table for one engine tree, and check the manifest.

The plugin targets exactly one engine version: the layout of GDScript's compiled
form is compiled into the binary, so one build describes one engine. Moving to a
new stable release is therefore this script plus a rebuild -- not a port.

It also cross-checks that the .gdextension's `compatibility_minimum` agrees with
the version the tables were generated from. Those are two separate places that
have to name the same engine, and when they disagree the failure is confusing:
the editor loads the plugin on a version the layout gate then refuses.

Usage:
    python3 tools/regen.py --godot /path/to/godot          # regenerate
    python3 tools/regen.py --godot /path/to/godot --check  # verify, do not write
"""

import argparse
import os
import re
import subprocess
import sys

ap = argparse.ArgumentParser()
ap.add_argument("--godot", required=True, help="engine source root")
ap.add_argument("--check", action="store_true", help="verify instead of writing")
ap.add_argument("--manifest", default="project/bin/example.gdextension")
a = ap.parse_args()

GENERATORS = ["gen_layout", "gen_opcodes", "gen_instruction_sizes", "gen_operand_plan"]
HERE = os.path.dirname(os.path.abspath(__file__))

failed = []
for name in GENERATORS:
    cmd = [sys.executable, os.path.join(HERE, name + ".py"), "--godot", a.godot]
    if a.check:
        cmd.append("--check")
    proc = subprocess.run(cmd, capture_output=True, text=True)
    status = "ok" if proc.returncode == 0 else "FAILED"
    line = (proc.stdout.strip().splitlines() or [""])[-1]
    print("%-26s %s  %s" % (name, status, line))
    if proc.returncode != 0:
        failed.append(name)
        if proc.stderr.strip():
            print(proc.stderr.strip())

# The version lives in two places that must agree.
layout = open(os.path.join(HERE, os.pardir, "src/bytecode/engine_layout.h")).read()
m = re.search(r'kLayoutEngineVersion = "([^"]+)"', layout)
generated_for = m.group(1) if m else None

manifest_version = None
if os.path.exists(a.manifest):
    mm = re.search(r'compatibility_minimum\s*=\s*"([^"]+)"', open(a.manifest).read())
    manifest_version = mm.group(1) if mm else None

print()
print("layout generated for   : %s" % generated_for)
print("manifest minimum       : %s" % manifest_version)
if generated_for != manifest_version:
    print("MISMATCH: the editor would load the plugin on a version the layout gate"
          " then refuses. Set compatibility_minimum to %r." % generated_for)
    failed.append("manifest")

sys.exit(1 if failed else 0)
