#include "disassembler.h"

#include "operand_plan.h"

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
namespace gdscript_disassemble {

// Address operands pack a 24-bit index with a 2-bit kind in the high bits; this
// is the only bit-packing in the whole instruction format.
static constexpr int kAddrBits = 24;
static constexpr int kAddrMask = (1 << kAddrBits) - 1;
static constexpr int kAddrTypeStack = 0;
static constexpr int kAddrTypeConstant = 1;
static constexpr int kAddrTypeMember = 2;

static constexpr int kStackSelf = 0;
static constexpr int kStackClass = 1;
static constexpr int kStackNil = 2;

// StringName does not concatenate onto String, so funnel name lookups through
// these.
static String gname(const FunctionView &p_function, int p_index) {
	return String(p_function.get_global_name(p_index));
}

static String mname(const FunctionView &p_function, int p_index) {
	return String(p_function.get_method_name(p_index));
}

static String quote_variant(const Variant &p_value) {
	switch (p_value.get_type()) {
		case Variant::STRING:
			return "\"" + String(p_value) + "\"";
		case Variant::STRING_NAME:
			return "&\"" + String(p_value) + "\"";
		case Variant::NODE_PATH:
			return "^\"" + String(p_value) + "\"";
		case Variant::OBJECT:
			// The payload is an *engine* object pointer. Both ways of resolving
			// it were tried and both crash when called from inside script
			// execution: object_get_instance_binding deadlocks ("mutex lock
			// failed") and object_get_class_name faults (SIGBUS). Reading the
			// name out of the object would need a class check first, and there
			// is no safe way to get one here -- so the value stays opaque rather
			// than taking the editor down for a nicer label.
			return String("object(?)");
		default:
			return String(p_value);
	}
}

String render_address(const ScriptMirror &p_script, const FunctionView &p_function, int p_operand) {
	const int index = p_operand & kAddrMask;
	switch (p_operand >> kAddrBits) {
		case kAddrTypeStack:
			switch (index) {
				case kStackSelf:
					return "self";
				case kStackClass:
					return "class";
				case kStackNil:
					return "nil";
				default:
					return String("stack(") + String::num_int64(index) + String(")");
			}
		case kAddrTypeConstant:
			return "const(" + quote_variant(p_function.get_constant(index)) + String(")");
		case kAddrTypeMember: {
			const String name = p_script.valid() ? p_script.member_name_by_index(index) : String();
			return name.is_empty() ? String("member(#") + String::num_int64(index) + String(")") : "member(" + name + String(")");
		}
		default:
			return String("#") + String::num_int64(p_operand);
	}
}

// Renders a `Variant::Type` operand. The engine's own table lives on the
// Variant class, which godot-cpp mirrors, so we can ask it directly.
static String type_name(int p_type) {
	if (p_type < 0 || p_type > Variant::VARIANT_MAX) {
		return String("type(#") + String::num_int64(p_type) + String(")");
	}
	return Variant::get_type_name((Variant::Type)p_type);
}

namespace {

struct Renderer {
	const FunctionView &func;
	const ScriptMirror &script;
	const int *code;
	int count;

	Renderer(const FunctionView &p_func, const ScriptMirror &p_script, const int *p_code, int p_count) :
			func(p_func), script(p_script), code(p_code), count(p_count) {}

	// Read tracking, used only by the operand self-test. `addr` deliberately does
	// not route through `slot`: doing that would record every address read as an
	// int read as well, and the comparison against the engine's declared kinds
	// would then fail on every instruction.
	mutable uint32_t read_addr = 0;
	mutable uint32_t read_imm = 0;

	static uint32_t bit(int p_offset) {
		return (p_offset >= 0 && p_offset < 32) ? (1u << p_offset) : 0u;
	}

	int raw(int p_ip, int p_offset) const {
		const int at = p_ip + p_offset;
		return (at >= 0 && at < count) ? code[at] : 0;
	}

	int slot(int p_ip, int p_offset) const {
		read_imm |= bit(p_offset);
		return raw(p_ip, p_offset);
	}

	String addr(int p_ip, int p_offset) const {
		read_addr |= bit(p_offset);
		return render_address(script, func, raw(p_ip, p_offset));
	}

	// The argument list of a variadic instruction. argc is not N: N also covers
	// the base/destination and count slots, so the two differ.
	String args_text(const OperandPlan &p_plan) const {
		const int argc = operand_at(p_plan.argc);
		String out;
		for (int i = 0; i < argc; i++) {
			out += (i > 0 ? String(", ") : String()) + addr_at(p_plan.args + i * p_plan.args_step);
		}
		return out;
	}

	// A typed container's element/key/value type. The engine's precedence is
	// script constant, then native class name, then built-in Variant type.
	String type_descriptor_script(int p_script_slot) const {
		// See the OBJECT branch of quote_variant: a script constant holds an
		// engine pointer that cannot be safely resolved from here.
		return p_script_slot >= 0 ? String("script(?)") : String();
	}

	String type_descriptor(int p_script_slot, int p_builtin_slot, int p_native_slot) const {
		// The engine's precedence is script > native > built-in, but the script
		// case cannot be resolved here (see type_descriptor_script), so it is
		// checked last: whatever it would have said is reported as unknown only
		// when nothing else names the type.
		const String script_name = type_descriptor_script(p_script_slot);
		if (!script_name.is_empty()) {
			return script_name;
		}
		if (p_native_slot >= 0) {
			const String native = String(func.get_global_name(operand_at(p_native_slot)));
			if (!native.is_empty()) {
				return native;
			}
		}
		if (p_builtin_slot >= 0) {
			const String builtin = type_name(operand_at(p_builtin_slot));
			if (builtin != String("Nil")) {
				return builtin;
			}
		}
		return String();
	}

	// Absolute slot access, for operands whose position comes from the generated
	// operand plan rather than from a literal offset.
	int operand_at(int p_slot) const {
		return (p_slot >= 0 && p_slot < count) ? code[p_slot] : 0;
	}
	String addr_at(int p_slot) const {
		return render_address(script, func, operand_at(p_slot));
	}

	// `ip` here is the opcode slot, matching the engine's own indexing.
	String render(int p_op, int p_ip) {
		// Operands are fetched lazily, per case. Reading them up front marked
		// slots 1..3 as read for *every* instruction, which is what made the
		// operand self-test report every instruction as disagreeing.
		switch (p_op) {
			case OPCODE_OPERATOR:
				return addr(p_ip, 3) + String(" = ") + addr(p_ip, 1) + String(" ") +
						func.debug_name(kLayout.kOff_GDScriptFunction_operator_names, slot(p_ip, 4)) + String(" ") +
						addr(p_ip, 2);

			case OPCODE_OPERATOR_VALIDATED:
				return addr(p_ip, 3) + String(" = ") + addr(p_ip, 1) + String(" ") +
						func.debug_name(kLayout.kOff_GDScriptFunction_operator_names, slot(p_ip, 4)) + String(" ") +
						addr(p_ip, 2);

			case OPCODE_ASSIGN:
				return addr(p_ip, 1) + String(" = ") + addr(p_ip, 2);
			case OPCODE_ASSIGN_TYPED_ARRAY:
			case OPCODE_ASSIGN_TYPED_DICTIONARY:
				// Found by the operand self-test: these had no case at all and
				// fell through to the raw-slot dump. The engine reads only the two
				// addresses here -- the element/key/value descriptors live in the
				// remaining slots and its own disassembler ignores them too.
				return addr(p_ip, 1) + String(" = ") + addr(p_ip, 2) +
						(p_op == OPCODE_ASSIGN_TYPED_ARRAY ? String("  (typed array)") : String("  (typed dictionary)"));
			case OPCODE_ASSIGN_NULL:
				return addr(p_ip, 1) + String(" = null");
			case OPCODE_ASSIGN_TRUE:
				return addr(p_ip, 1) + String(" = true");
			case OPCODE_ASSIGN_FALSE:
				return addr(p_ip, 1) + String(" = false");

			case OPCODE_ASSIGN_TYPED_BUILTIN:
			case OPCODE_CAST_TO_BUILTIN:
				return addr(p_ip, p_op == OPCODE_CAST_TO_BUILTIN ? 1 : 1) + String(" = ") +
						addr(p_ip, 2) + String(" as ") + type_name(slot(p_ip, 3)) + (p_op == OPCODE_CAST_TO_BUILTIN ? " (cast)" : "");
			case OPCODE_ASSIGN_TYPED_NATIVE:
			case OPCODE_CAST_TO_NATIVE:
				return addr(p_ip, 1) + String(" = ") + addr(p_ip, 2) + String(" as native ") +
						quote_variant(func.get_constant(slot(p_ip, 3) & kAddrMask));
			case OPCODE_ASSIGN_TYPED_SCRIPT:
			case OPCODE_CAST_TO_SCRIPT:
				return addr(p_ip, 1) + String(" = ") + addr(p_ip, 2) + String(" as script ") +
						quote_variant(func.get_constant(slot(p_ip, 3) & kAddrMask));

			case OPCODE_TYPE_TEST_BUILTIN:
				return addr(p_ip, 1) + String(" = ") + addr(p_ip, 2) + String(" is ") + type_name(slot(p_ip, 3));
			case OPCODE_TYPE_TEST_NATIVE:
				return addr(p_ip, 1) + String(" = ") + addr(p_ip, 2) + String(" is native ") + gname(func, slot(p_ip, 3));
			case OPCODE_TYPE_TEST_SCRIPT:
				return addr(p_ip, 1) + String(" = ") + addr(p_ip, 2) + String(" is script ") +
						quote_variant(func.get_constant(slot(p_ip, 3) & kAddrMask));

			case OPCODE_GET_MEMBER:
				return addr(p_ip, 1) + String(" = ") + gname(func, slot(p_ip, 2));
			case OPCODE_SET_MEMBER:
				return gname(func, slot(p_ip, 2)) + String(" = ") + addr(p_ip, 1);
			case OPCODE_GET_STATIC_VARIABLE:
				return addr(p_ip, 1) + String(" = static#") + String::num_int64(slot(p_ip, 3));
			case OPCODE_SET_STATIC_VARIABLE:
				return String("static#") + String::num_int64(slot(p_ip, 3)) + String(" = ") + addr(p_ip, 1);

			case OPCODE_GET_KEYED:
				return addr(p_ip, 3) + String(" = ") + addr(p_ip, 1) + String("[") + addr(p_ip, 2) + String("]");
			case OPCODE_SET_KEYED:
				return addr(p_ip, 1) + String("[") + addr(p_ip, 2) + String("] = ") + addr(p_ip, 3);
			case OPCODE_GET_KEYED_VALIDATED:
			case OPCODE_GET_INDEXED_VALIDATED:
				return addr(p_ip, 3) + String(" = ") + addr(p_ip, 1) + String("[") + addr(p_ip, 2) + String("]");
			case OPCODE_SET_KEYED_VALIDATED:
			case OPCODE_SET_INDEXED_VALIDATED:
				return addr(p_ip, 1) + String("[") + addr(p_ip, 2) + String("] = ") + addr(p_ip, 3);

			case OPCODE_GET_NAMED:
			case OPCODE_GET_NAMED_VALIDATED:
				return addr(p_ip, 2) + String(" = ") + addr(p_ip, 1) + String(".") + gname(func, slot(p_ip, 3));
			case OPCODE_SET_NAMED:
			case OPCODE_SET_NAMED_VALIDATED:
				return addr(p_ip, 1) + String(".") + gname(func, slot(p_ip, 3)) + String(" = ") + addr(p_ip, 2);

			case OPCODE_CALL:
			case OPCODE_CALL_RETURN:
			case OPCODE_CALL_ASYNC: {
				// [op][N][args..][base][dst][argc][name_idx]
				// The argument count is not N: N also covers the base/destination and
				// count slots. argc is read from the position the engine uses.
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				const int argc = operand_at(plan.argc);
				String args;
				for (int i = 0; i < argc; i++) {
					args += (i > 0 ? String(", ") : String()) + addr_at(plan.args + i);
				}
				const String base = addr_at(plan.dst2);
				const String dst = addr_at(plan.dst);
				const String method = gname(func, operand_at(plan.payload));
				const bool returns = p_op != OPCODE_CALL;
				return (returns ? dst + String(" = ") : String()) + base + String(".") + method + String("(") + args + String(")");
			}

			case OPCODE_CALL_METHOD_BIND:
			case OPCODE_CALL_METHOD_BIND_VALIDATED_RETURN: {
				// The argument count is not N: N also covers the base/destination and
				// count slots. argc is read from the position the engine uses.
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				const int argc = operand_at(plan.argc);
				String args;
				for (int i = 0; i < argc; i++) {
					args += (i > 0 ? String(", ") : String()) + addr_at(plan.args + i);
				}
				const String base = addr_at(plan.dst2);
				const String dst = addr_at(plan.dst);
				const int method_index = operand_at(plan.payload);
				const String method = mname(func, method_index);
				return dst + String(" = ") + base + String(".") +
						(method.is_empty() ? String("bind#") + String::num_int64(method_index) : method) +
						"(" + args + String(")");
			}

			case OPCODE_CALL_UTILITY: {
				// The argument count is not N: N also covers the base/destination and
				// count slots. argc is read from the position the engine uses.
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				const int argc = operand_at(plan.argc);
				String args;
				for (int i = 0; i < argc; i++) {
					args += (i > 0 ? String(", ") : String()) + addr_at(plan.args + i);
				}
				return addr_at(plan.dst) + String(" = ") + gname(func, operand_at(plan.payload)) + String("(") + args + String(")");
			}

			case OPCODE_CALL_UTILITY_VALIDATED:
			case OPCODE_CALL_GDSCRIPT_UTILITY: {
				// The argument count is not N: N also covers the base/destination and
				// count slots. argc is read from the position the engine uses.
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				const int argc = operand_at(plan.argc);
				String args;
				for (int i = 0; i < argc; i++) {
					args += (i > 0 ? String(", ") : String()) + addr_at(plan.args + i);
				}
				const int index = operand_at(plan.payload);
				const size_t table = (p_op == OPCODE_CALL_GDSCRIPT_UTILITY)
						? kLayout.kOff_GDScriptFunction_gds_utilities_names
						: kLayout.kOff_GDScriptFunction_utilities_names;
				const String name = func.debug_name(table, index);
				return addr_at(plan.dst) + String(" = ") +
						(name.is_empty() ? String("utility#") + String::num_int64(index) : name) +
						"(" + args + String(")");
			}

			case OPCODE_CONSTRUCT: {
				// [op][N][args..][dst][argc][Variant::Type]
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				return addr_at(plan.dst) + String(" = ") +
						type_name(operand_at(plan.payload)) + String("(") + args_text(plan) + String(")");
			}
			case OPCODE_CONSTRUCT_VALIDATED: {
				// Same shape, but the trailing operand indexes the engine's own
				// constructor-name table instead of naming a Variant type.
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				const int ctor = operand_at(plan.payload);
				const String name = func.debug_name(kLayout.kOff_GDScriptFunction_constructors_names, ctor);
				return addr_at(plan.dst) + String(" = ") +
						(name.is_empty() ? String("ctor#") + String::num_int64(ctor) : name) +
						"(" + args_text(plan) + String(")");
			}
			case OPCODE_CONSTRUCT_ARRAY: {
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				return addr_at(plan.dst) + String(" = [") + args_text(plan) + String("]");
			}
			case OPCODE_CONSTRUCT_TYPED_ARRAY: {
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				const String elem = type_descriptor(plan.elem_script, plan.elem_builtin, plan.elem_native);
				return addr_at(plan.dst) + String(" = ") + elem +
						String("[") + args_text(plan) + String("]");
			}
			case OPCODE_CONSTRUCT_TYPED_DICTIONARY: {
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				const int argc = operand_at(plan.argc);
				String items;
				for (int i = 0; i < argc; i++) {
					const int base = plan.args + i * plan.args_step;
					items += (i > 0 ? String(", ") : String()) + addr_at(base) + String(": ") + addr_at(base + 1);
				}
				const String key = type_descriptor(plan.key_script, plan.key_builtin, plan.key_native);
				const String value = type_descriptor(plan.value_script, plan.value_builtin, plan.value_native);
				return addr_at(plan.dst) + String(" = ") + key + String("->") + value +
						String("{") + items + String("}");
			}
			case OPCODE_CONSTRUCT_DICTIONARY: {
				// Dictionary operands come in key/value pairs, so the stride is two.
				const OperandPlan plan = operand_plan(p_op, code, p_ip);
				const int argc = operand_at(plan.argc);
				String items;
				for (int i = 0; i < argc; i++) {
					const int base = plan.args + i * plan.args_step;
					items += (i > 0 ? String(", ") : String()) + addr_at(base) + String(": ") + addr_at(base + 1);
				}
				return addr_at(plan.dst) + String(" = {") + items + String("}");
			}

			case OPCODE_JUMP:
				return String("jump -> ") + String::num_int64(slot(p_ip, 1));
			case OPCODE_JUMP_IF:
				return "if " + addr(p_ip, 1) + String(" -> ") + String::num_int64(slot(p_ip, 2));
			case OPCODE_JUMP_IF_NOT:
				return "if not " + addr(p_ip, 1) + String(" -> ") + String::num_int64(slot(p_ip, 2));
			case OPCODE_JUMP_IF_SHARED:
				return "if shared " + addr(p_ip, 1) + String(" -> ") + String::num_int64(slot(p_ip, 2));
			case OPCODE_JUMP_TO_DEF_ARGUMENT:
				return "jump to default argument";

			case OPCODE_RETURN:
				return "return " + addr(p_ip, 1);
			case OPCODE_RETURN_TYPED_BUILTIN:
				return "return " + addr(p_ip, 1) + String(" as ") + type_name(slot(p_ip, 2));
			case OPCODE_RETURN_TYPED_NATIVE:
				// Slot 2 is an address holding the native class constant, not a
				// global-name index -- the engine prints it as DADDR(2).
				return "return " + addr(p_ip, 1) + String(" as native ") + addr(p_ip, 2);
			case OPCODE_RETURN_TYPED_ARRAY:
			case OPCODE_RETURN_TYPED_DICTIONARY:
				// The instruction also carries the element (or key/value) type
				// descriptors, but the engine's own disassembler does not read
				// them -- it prints only the value. Showing the type would mean
				// deriving those slots from the VM rather than from the
				// disassembler, so this matches the engine rather than guessing.
				return "return " + addr(p_ip, 1) +
						(p_op == OPCODE_RETURN_TYPED_ARRAY ? String(" as Array") : String(" as Dictionary"));
			case OPCODE_RETURN_TYPED_SCRIPT:
				return "return " + addr(p_ip, 1) + String(" as script ") + quote_variant(func.get_constant(slot(p_ip, 2) & kAddrMask));

			case OPCODE_AWAIT:
				return "await " + addr(p_ip, 1);
			case OPCODE_AWAIT_RESUME:
				return addr(p_ip, 1) + String(" = await resumed");

			case OPCODE_LINE:
				return String("line ") + String::num_int64(slot(p_ip, 1));
			case OPCODE_END:
				return "end";
			case OPCODE_BREAKPOINT:
				return "breakpoint";
			case OPCODE_ASSERT:
				return "assert " + addr(p_ip, 1) + String(", ") + addr(p_ip, 2);

			case OPCODE_STORE_GLOBAL:
				return addr(p_ip, 1) + String(" = global(") + gname(func, slot(p_ip, 2)) + String(")");
			case OPCODE_STORE_NAMED_GLOBAL:
				return addr(p_ip, 1) + String(" = named_global(") + gname(func, slot(p_ip, 2)) + String(")");

			case OPCODE_CREATE_LAMBDA:
			case OPCODE_CREATE_SELF_LAMBDA: {
				// Engine layout (there `ip` is already past the opcode, so every
				// offset shifts by one):
				//   [op][N][captures..][dst][captures_count][lambda_idx]
				//     N              = code[ip0+1]
				//     captures_count = code[ip0+2+N]   <- NOT N
				//     lambda index   = code[ip0+3+N]
				// Reading the lambda index one slot too far produced a wild
				// `lambdas[]` dereference that crashed the editor.
				const int n = slot(p_ip, 1);
				const int captures_count = slot(p_ip, 2 + n);
				const int lambda_index = slot(p_ip, 3 + n);

				String captures;
				for (int i = 0; i < captures_count; i++) {
					captures += (i > 0 ? ", " : "") + addr(p_ip, 1 + i);
				}

				String name = String("lambda#") + String::num_int64(lambda_index);
				const void *const *lambdas = func.lambdas();
				// Bounds-check against the table's real size before dereferencing.
				// An unchecked index reads arbitrary memory and then copies a
				// StringName out of it, which aborts the whole editor.
				if (lambdas != nullptr && lambda_index >= 0 && lambda_index < func.lambda_count()) {
					FunctionView lf(lambdas[lambda_index]);
					if (lf.valid() && lf.looks_like_valid_code()) {
						name = String(lf.get_name());
					}
				}

				const String dst = addr(p_ip, 1 + captures_count);
				return dst + String(" = ") + name + (captures.is_empty() ? "()" : "(" + captures + String(")"));
			}

			default:
				break;
		}

		// Iteration and the type-adjust ops all share one shape:
		// [op][counter][container][dst][end], with the range variants wider.
		if (p_op == OPCODE_ITERATE_BEGIN_RANGE) {
			// [op][counter][from][to][step][dst][end]
			return "for-init " + addr(p_ip, 5) + String(" in range ") + addr(p_ip, 2) + String("..") + addr(p_ip, 3) +
					String(" step ") + addr(p_ip, 4) + String(" counter ") + addr(p_ip, 1) +
					String(" end ") + String::num_int64(slot(p_ip, 6));
		}
		if (p_op == OPCODE_ITERATE_RANGE) {
			// One slot narrower than the for-init form, and there is no second
			// bound: the engine prints `in range to <to> step <step>`. Reading the
			// step as an upper bound -- which this did -- is a different operand.
			// [op][counter][to][step][dst][end]
			return "for " + addr(p_ip, 4) + String(" in range to ") + addr(p_ip, 2) +
					String(" step ") + addr(p_ip, 3) + String(" counter ") + addr(p_ip, 1) +
					String(" end ") + String::num_int64(slot(p_ip, 5));
		}
		if ((p_op >= OPCODE_ITERATE_BEGIN_INT && p_op <= OPCODE_ITERATE_BEGIN_OBJECT) ||
				(p_op >= OPCODE_ITERATE_INT && p_op <= OPCODE_ITERATE_OBJECT)) {
			const bool is_begin = p_op < OPCODE_ITERATE_INT;
			return String(is_begin ? "for-init " : "for-loop ") + addr(p_ip, 3) + String(" in ") + addr(p_ip, 2) +
					" counter " + addr(p_ip, 1) + String(" end ") + String::num_int64(slot(p_ip, 4));
		}

		// Anything left: show the operands as raw numbers rather than guess. The
		// length came from the generated table, so the listing stays aligned even
		// when the wording is plain.
		String out = opcode_name(p_op);
		const int len = instruction_length(p_op, code, p_ip);
		for (int i = 1; i < len; i++) {
			out += String(" ") + String::num_int64(slot(p_ip, i));
		}
		return out;
	}
};

} // namespace

FunctionListing disassemble_function(const FunctionView &p_function, const ScriptMirror &p_script, const String &p_path) {
	FunctionListing listing;
	listing.path = p_path;
	if (!p_function.valid()) {
		listing.error = "Function is not readable.";
		return listing;
	}

	listing.name = p_function.get_name();
	listing.argument_count = p_function.get_argument_count();
	listing.is_vararg = p_function.is_vararg();
	listing.is_static = p_function.is_static();
	listing.initial_line = p_function.get_initial_line();

	const int *code = p_function.code();
	const int size = p_function.code_size();
	if (code == nullptr || size <= 0) {
		listing.error = "Function has no code.";
		return listing;
	}
	// Refuse to walk anything that does not structurally look like a compiled
	// function. Without this, a wrong offset turns into a wild pointer walk.
	if (!p_function.looks_like_valid_code()) {
		listing.error = "Function does not look like compiled bytecode; refusing to read it.";
		return listing;
	}

	Renderer renderer(p_function, p_script, code, size);

	int ip = 0;
	while (ip < size) {
		const int opcode = code[ip];
		if (opcode < 0 || opcode >= kOpcodeCount) {
			listing.error = vformat("Unknown opcode %d at %d; stopping.", opcode, ip);
			return listing;
		}
		const int length = instruction_length(opcode, code, ip);
		if (length <= 0 || ip + length > size) {
			listing.error = vformat("Instruction %s at %d does not fit the stream; stopping.",
					opcode_name(opcode), ip);
			return listing;
		}

		DecodedInstruction ins;
		ins.ip = ip;
		ins.opcode = opcode;
		ins.length = length;
		ins.text = renderer.render(opcode, ip);

		// The line a source line applies to. Applying it *to* the OPCODE_LINE that
		// carries it is what makes functions whose initial line is 0 (the
		// synthesised `@implicit_initializer`) show a line at all.
		if (opcode == OPCODE_LINE) {
			ins.source_line = code[ip + 1];
		}

		listing.instructions.push_back(ins);

		ip += length;
	}

	listing.ok = true;
	return listing;
}


// TEMPORARY DIAGNOSTIC -- not part of the product path. Isolates which step of
// resolving an object constant faults, by doing one step more each run and
// printing before each so the output survives a crash.
String probe_object_names(Script *p_script, int p_step) {
	ScriptMirror mirror;
	String err;
	if (!ScriptMirror::bind(p_script, mirror, err)) {
		return err;
	}
	List<ScriptMirror::Entry> entries;
	mirror.collect_functions(entries, String(p_script->get_global_name()));

	int seen = 0;
	for (const ScriptMirror::Entry &E : entries) {
		const FunctionView &f = E.function;
		const int n = f.constant_count();
		for (int i = 0; i < n; i++) {
			const Variant v = f.get_constant(i);
			if (v.get_type() != Variant::OBJECT) {
				continue;
			}
			seen++;
			UtilityFunctions::print("const ", i, " in ", E.path);

			UtilityFunctions::print("  step0: taking the pointer...");
			const void *obj = v.operator Object *();
			UtilityFunctions::print("  step0 ok ptr=", (uint64_t)obj);

			if (p_step < 1) {
				continue;
			}
			UtilityFunctions::print("  step1: object_get_class_name...");
			alignas(StringName) unsigned char storage[sizeof(StringName)];
			const bool ok = ::godot::gdextension_interface::object_get_class_name(
					obj, ::godot::gdextension_interface::library,
					reinterpret_cast<GDExtensionUninitializedStringNamePtr>(storage));
			UtilityFunctions::print("  step1 ok=", (int)ok);
			if (!ok) {
				continue;
			}
			const String cls = String(*reinterpret_cast<const StringName *>(storage));
			UtilityFunctions::print("  step1 class=", cls);

			if (p_step < 2) {
				continue;
			}
			UtilityFunctions::print("  step2: local_name...");
			if (cls == String("GDScript")) {
				const StringName local = *reinterpret_cast<const StringName *>(
						static_cast<const char *>(obj) + kLayout.kOff_GDScript_local_name);
				UtilityFunctions::print("  step2 ok local=", String(local));
			} else {
				UtilityFunctions::print("  step2 skipped (not a GDScript)");
			}

			if (p_step < 3) {
				continue;
			}
			UtilityFunctions::print("  step3: fully_qualified_name...");
			if (cls == String("GDScript")) {
				const String qualified = *reinterpret_cast<const String *>(
						static_cast<const char *>(obj) + kLayout.kOff_GDScript_fully_qualified_name);
				UtilityFunctions::print("  step3 ok fq=", qualified);
			}
		}
	}
	return vformat("probed %d object constants", seen);
}

String check_fixed_operands(Script *p_script) {
	ScriptMirror mirror;
	String error;
	if (!ScriptMirror::bind(p_script, mirror, error)) {
		return error;
	}

	List<ScriptMirror::Entry> entries;
	mirror.collect_functions(entries, String(p_script->get_global_name()));

	int checked = 0;
	int bad = 0;
	String report;
	for (const ScriptMirror::Entry &E : entries) {
		const ScriptMirror owner(E.owner);
		const int *code = E.function.code();
		const int size = E.function.code_size();
		if (code == nullptr || size <= 0) {
			continue;
		}
		Renderer renderer(E.function, owner, code, size);
		int ip = 0;
		while (ip < size) {
			const int op = code[ip];
			if (op < 0 || op >= kOpcodeCount) {
				break;
			}
			const int length = instruction_length(op, code, ip);
			if (length <= 0 || ip + length > size) {
				break;
			}

			renderer.read_addr = 0;
			renderer.read_imm = 0;
			renderer.render(op, ip);

			const FixedPlan plan = fixed_plan(op, ip);
			if (plan.addr_count > 0 || plan.imm_count > 0) {
				uint32_t want_addr = 0;
				uint32_t want_imm = 0;
				for (int i = 0; i < plan.addr_count; i++) {
					want_addr |= Renderer::bit(plan.addr[i] - ip);
				}
				for (int i = 0; i < plan.imm_count; i++) {
					want_imm |= Renderer::bit(plan.imm[i] - ip);
				}
				// Reading a slot the engine does not read, or reading it as the
				// wrong kind, both mean this case has drifted.
				const uint32_t excess = (renderer.read_addr & ~want_addr) | (renderer.read_imm & ~want_imm);
				checked++;
				if (excess != 0) {
					bad++;
					report += vformat("%s @%d %s: read addr=0x%x imm=0x%x, engine addr=0x%x imm=0x%x\n",
							E.path, ip, opcode_name(op), renderer.read_addr, renderer.read_imm, want_addr, want_imm);
				}
			}
			ip += length;
		}
	}

	if (bad == 0) {
		return String();
	}
	return vformat("%d of %d instructions disagree with the engine:\n%s", bad, checked, report);
}

bool disassemble_script(Script *p_script, String &r_text, String &r_error) {
	ScriptMirror mirror;
	if (!ScriptMirror::bind(p_script, mirror, r_error)) {
		return false;
	}

	List<ScriptMirror::Entry> entries;
	// Prefix nested classes with the script's own class name when it has one, so
	// paths read like `Player.Inner.f` rather than a bare `Inner.f`.
	mirror.collect_functions(entries, String(p_script->get_global_name()));

	String out;
	const String source = p_script->get_source_code();
	const String script_path = p_script->get_path().is_empty() ? String("<unsaved>") : String(p_script->get_path());

	out += "Script: " + script_path + "\n";
	out += vformat("Functions: %d   Layout: Godot %s\n", (int)entries.size(), kLayoutEngineVersion);
	out += String("=").repeat(72) + "\n";

	if (entries.is_empty()) {
		out += "\nNo compiled functions. The script may have failed to compile.\n";
		r_text = out;
		return true;
	}

	bool any_decode_failure = false;
	for (const ScriptMirror::Entry &E : entries) {
		// Resolve member operands against the class the function belongs to; for
		// nested classes that is not the script we started from.
		const ScriptMirror owner(E.owner);
		const FunctionListing listing = disassemble_function(E.function, owner, E.path);

		String signature = E.path;
		if (listing.is_static) {
			signature = String("static ") + signature;
		}
		if (!listing.ok) {
			any_decode_failure = true;
		}
		out += "\n";
		out += "--- " + signature + " ---\n";

		if (!listing.ok) {
			out += "  <decode failed: " + listing.error + ">\n";
			continue;
		}

		// Render each instruction on its own line, with the source line in a
		// fixed-width leading column so the listing scans vertically.
		int current_line = listing.initial_line;
		for (const DecodedInstruction &ins : listing.instructions) {
			if (ins.source_line > 0) {
				current_line = ins.source_line;
			}
			const String line_text = (current_line > 0) ? String::num_int64(current_line) : String("-");
			const String ip_text = String::num_int64(ins.ip);
			out += vformat("%6s %6s  %s\n", line_text, ip_text, ins.text);
		}
	}

	if (any_decode_failure) {
		out += "\nSome functions could not be decoded; the rest are shown above.\n";
	}

	r_text = out;
	return true;
}

} // namespace gdscript_disassemble
} // namespace godot
