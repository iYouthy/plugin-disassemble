# GDScript Bytecode Disassembler

A GDExtension editor plugin that shows the compiled GDScript bytecode of the
script you are editing, beside the code, with syntax highlighting.

Toggle it with the **Bytecode** button in the script editor's toolbar. The pane
opens as a third column of the script editor's own split, so it drags and resizes
like the file list and the code editor do.

```
    25      2  stack(5) = member(speed) + stack(3)
    26     12  class = <anonymous lambda>()
    28     22  stack(7) = stack(4).call(const(21))
    17     51  for-init stack(4) in range stack(6)..stack(7) step stack(8) counter stack(5) end 78
```

## Why this reads engine memory

Godot does not expose GDScript bytecode through any public API. This was checked
exhaustively before the design was settled:

| Route | Result |
|---|---|
| `GDScript` bound methods | only `new()` |
| `ScriptLanguage` bound methods | none at all |
| `ScriptLanguage::debug_get_function_bytecode` | has never existed in the engine |
| `EngineDebugger.send_message("get_function_bytecode")` | no such message, and `is_active()` is false in an editor process |
| debugger protocol | fixed command set, nothing bytecode-shaped |
| `--test gdscript-bytecode` | prints `"Not implemented."` in both 4.7.2 and 4.8-dev |
| dynamic symbols | the editor binary's Godot symbols are stripped |

The compiled form lives only in `GDScriptFunction` objects in memory. That class
is not a ClassDB class, has no public accessors, and is reachable only from
inside the engine. So the plugin takes the raw engine pointer that godot-cpp
keeps for every wrapped object and reads the engine's own struct layout.

This is deliberately narrow: **the plugin only ever reads, never writes**, and
every entry point validates before it trusts what it found.

## The generators

The offsets are not written by hand. `tools/` holds four generators, each of
which reads them out of the engine's own source:

| Tool | Produces | Extracted from |
|---|---|---|
| `gen_layout.py` | `engine_layout.h` | engine headers, compiled with the engine's own flags |
| `gen_opcodes.py` | `opcodes.h` | `gdscript_function.h` opcode enum |
| `gen_instruction_sizes.py` | `instruction_sizes.h` | each case's advance in `gdscript_disassembler.cpp` |
| `gen_operand_plan.py` | `operand_plan.h` | operand reads in `gdscript_disassembler.cpp` |

`gen_layout.py` works by declaring `P<offsetof(T, member)>` for an undefined
template `P`, then compiling with `-fsyntax-only` and reading the values out of
clang's diagnostics. Nothing is linked and nothing is executed, so the probe
needs no `main()`, no engine library, and no running engine.

**Why mechanical.** Hand-transcribing these offsets produced four separate bugs
before the generators existed: a lambda index one slot too far (which crashed the
editor), a constructor index read off the wrong count, wrong operand slots for
the whole `CALL_*` family, and `N` confused with `argc`. Each was silent — the
listing looked plausible. Deriving them from the engine removes the class of
error rather than the instances.

Instruction *lengths* and operands are the two things that cannot be guessed:
instructions carry no length of their own, so one wrong length desynchronises
everything after it, and operand positions differ per opcode.

## The safety model

Four layers, because a wrong offset means reading arbitrary memory:

1. **Compile time** — `static_assert`s in `gdscript_mirror.h` compare godot-cpp's
   compiled container sizes against the sizes measured inside the engine. A
   future godot-cpp that changes one fails the build.
2. **Engine version gate** — the layout describes one engine build; `verify()`
   reports the mismatch instead of proceeding.
3. **Self-consistency** — every `member_functions` map key must equal the
   function's own `name` field, which validates the map offset, the iteration,
   and the name offset at once.
4. **Public cross-check** — the keys are compared against
   `Script::get_script_method_list()`, which is a real bound method and therefore
   independent of the layout table.

Per function, `looks_like_valid_code()` additionally refuses to walk anything
that does not end in `OPCODE_END` with a valid opcode first.

## Building

```bash
git submodule update --init --recursive
scons arch=arm64 debug_symbols=yes      # macOS arm64; see SConstruct for api_version
```

The extension lands in `project/bin/` and is loaded by the `project/` test
project. `reloadable = false`, so **restart the editor** after rebuilding.

## Regenerating against a different engine

The committed tables describe **Godot 4.7.2 built with `TOOLS_ENABLED` and
`DEBUG_ENABLED`**. Offsets differ per version *and* per build configuration, so a
different engine needs the tables regenerated:

```bash
for t in gen_layout gen_opcodes gen_instruction_sizes gen_operand_plan; do
    python3 tools/$t.py --godot /path/to/godot
done
```

Each generator takes `--check` to compare instead of writing, which is the way to
detect a table that has gone stale:

```bash
python3 tools/gen_layout.py --godot /path/to/godot --check
```

Regenerating needs the target tree to have been configured for building (it reads
`compile_commands.json` for the exact compile flags).

## Tests

The suite runs headlessly against the `project/` test project:

```bash
GODOT=/path/to/godot.macos.editor.arm64
$GODOT --headless --path project --script tests/test_edge.gd   # assertions, exit code
$GODOT --headless --path project --script tests/test_hl.gd     # highlighter tokeniser
```

`test_edge.gd` is the one that asserts. Besides the feature checks, it carries a
**fallback probe**: unresolved operands render as `member(#`, `ctor#`, `type(#`,
`utility#`, `bind#`, `lambda#`. Those appear exactly when an operand index is out
of range, which is what a wrong slot produces — so the probe is a direct check
that the operand positions still agree with the engine.

The other scripts in `tests/` only print, and are there for looking at output by
eye.

## Interpreting a listing

A call can be missing from the listing, and that is not a decoding error. GDScript
folds calls to **built-in utility functions** when the arguments are constant and
the result is unused, and then drops the statement as a dead pure expression. So
`absf(1)` or `abs(1)` written as a bare statement produce no instruction at all,
while `print(1)` does, because `print` has side effects.

This bites hardest when a script declares a method whose name collides with a
built-in utility. A declared `func absf(i: int)` does *not* shadow the built-in
`absf`, so `absf(1)` still resolves to the utility and disappears — rename the
method and the call appears. Verified against the raw instruction stream, not
just the rendered listing, so it is the engine's output and not a decoder bug.

## Known limitations

- **Object constants are opaque.** A `preload(...)` or a script type in a typed
  container renders as `object(?)` / `script(?)`. Both ways of resolving an
  engine object pointer to something nameable were tried and both crash when
  called from inside script execution: `object_get_instance_binding` deadlocks
  (`mutex lock failed`) and `object_get_class_name` faults (SIGBUS). Reading the
  name as a field would need a class check first, and there is no safe way to get
  one here. Built-in type descriptors (`Array[String]`) are unaffected.
- **Fixed-shape opcodes are still hand-written.** `gen_operand_plan.py` extracts
  their operand reads into `fixed_plan()`, and that extraction already proved one
  bug real (`RETURN_TYPED_NATIVE` reads no int operand, so treating a slot as a
  global-name index was wrong). But the renderer does not consume `fixed_plan`
  yet, so those cases are not mechanically guaranteed the way the variadic ones
  are.
- **`RETURN_TYPED_ARRAY` / `_DICTIONARY` do not show the element type.** The
  engine's own disassembler does not read those slots either. Showing them would
  mean deriving the positions from the VM rather than the disassembler.
- **Nested `class Inner:` bodies are walked**, but the element types of their
  typed containers follow the same object-constant limitation above.
- Layout tables are verified for one engine build only (see above).
