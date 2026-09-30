// Turns a FunctionView's instruction stream into something readable.
//
// Instruction *lengths* come from the generated table (tools/gen_instruction_sizes.py),
// which is extracted from the engine's own disassembler -- so the walk can never
// desynchronise. Operand *rendering* is done here and is deliberately allowed to
// degrade to raw values for shapes we do not special-case: a cosmetically plain
// line is fine, a misaligned listing is not.

#pragma once

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/list.hpp>

#include "gdscript_mirror.h"
#include "instruction_sizes.h"

namespace godot {
namespace gdscript_disassemble {

struct DecodedInstruction {
	int ip = 0;
	int opcode = 0;
	int length = 0;
	int source_line = -1; // 1-based, only set for OPCODE_LINE
	String text;
};

struct FunctionListing {
	String path; // "ClassName.method", used as the panel heading
	StringName name;
	int argument_count = 0;
	bool is_static = false;
	bool is_vararg = false;
	int initial_line = 0;
	bool ok = false;
	String error; // set when ok == false
	Vector<DecodedInstruction> instructions;
};

// Decodes one function. Never throws and never walks off the end: a length that
// would overrun the stream stops Decoding and reports what went wrong.
FunctionListing disassemble_function(const FunctionView &p_function, const ScriptMirror &p_script, const String &p_path);

// Every function of a script, plus a rendering of each. Returns false and fills
// r_error when the script's bytecode cannot be trusted (layout mismatch, bad
// stream), in which case r_text holds the explanation instead of a listing.
bool disassemble_script(Script *p_script, String &r_text, String &r_error);

// Verifies the hand-written fixed-shape cases against the operand reads the
// engine declares, using the generated fixed_plan. Returns an empty string when
// every instruction agreed, or a report of the ones that did not.
//
// This is a test hook, deliberately separate from rendering: an earlier attempt
// embedded the comparison in the render path and it made normal output
// unreadable when it mis-fired.
String check_fixed_operands(Script *p_script);

// TEMPORARY DIAGNOSTIC.
String dump_raw_code(Script *p_script, const String &p_function);

// TEMPORARY DIAGNOSTIC.
String probe_object_names(Script *p_script, int p_step);

// Renders an address operand (`index | type << 24`) the way the engine's own
// disassembler does: self / stack(n) / const(...) / member(...).
String render_address(const ScriptMirror &p_script, const FunctionView &p_function, int p_operand);

} // namespace gdscript_disassemble
} // namespace godot
