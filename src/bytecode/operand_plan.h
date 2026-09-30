// GENERATED FILE -- do not edit by hand.
// Regenerate with: python3 tools/gen_operand_plan.py --godot <engine-source>
//
// Where each variadic instruction keeps its argument count, arguments,
// destination, and trailing index operand. Read out of the engine's own
// disassembler so the plugin cannot disagree with it.

#pragma once

#include "opcodes.h"

namespace godot {
namespace gdscript_disassemble {

// All slots are relative to `ip0`, the opcode's own index, and are resolved
// against the live instruction stream because the trailing operands sit at
// a position that depends on the argument count. A value of -1 means the
// opcode does not have that operand.
struct OperandPlan {
	int argc = -1;    // slot holding the real argument count
	int args = -1;      // slot of the first argument
	int args_step = 1;  // stride between arguments (2 for dictionaries)
	int dst = -1;     // first count-indexed address (usually the destination)
	int dst2 = -1;      // second count-indexed address, where the opcode has one
	int payload = -1; // slot of the trailing index operand (method, ctor, ...)

	// Element/key/value type descriptors for the typed container ops:
	// script constant address, Variant::Type, and native name index.
	int elem_script = -1, elem_builtin = -1, elem_native = -1;
	int key_script = -1, key_builtin = -1, key_native = -1;
	int value_script = -1, value_builtin = -1, value_native = -1;
};

inline OperandPlan operand_plan(int p_opcode, const int *code, int ip0) {
	OperandPlan plan;
	switch (p_opcode) {
		case OPCODE_CALL: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 3 + code[plan.argc] * 1; plan.dst2 = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_ASYNC: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 3 + code[plan.argc] * 1; plan.dst2 = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_BUILTIN_STATIC: { plan.argc = ip0 + 4 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 2 + code[ip0 + 1]; } break;
		case OPCODE_CALL_BUILTIN_TYPE_VALIDATED: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 3 + code[plan.argc] * 1; plan.payload = ip0 + 5 + code[plan.argc]; } break;
		case OPCODE_CALL_GDSCRIPT_UTILITY: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 4 + code[plan.argc]; } break;
		case OPCODE_CALL_METHOD_BIND: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 3 + code[plan.argc] * 1; plan.dst2 = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_METHOD_BIND_RET: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 3 + code[plan.argc] * 1; plan.dst2 = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_METHOD_BIND_VALIDATED_NO_RETURN: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_METHOD_BIND_VALIDATED_RETURN: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 3 + code[plan.argc] * 1; plan.dst2 = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_NATIVE_STATIC: { plan.argc = ip0 + 3 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 2 + code[ip0 + 1]; } break;
		case OPCODE_CALL_NATIVE_STATIC_VALIDATED_NO_RETURN: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_NATIVE_STATIC_VALIDATED_RETURN: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_RETURN: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 3 + code[plan.argc] * 1; plan.dst2 = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_SELF_BASE: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 3 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_UTILITY: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CALL_UTILITY_VALIDATED: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 4 + code[plan.argc]; } break;
		case OPCODE_CONSTRUCT: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 4 + code[ip0 + 1]; } break;
		case OPCODE_CONSTRUCT_ARRAY: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; } break;
		case OPCODE_CONSTRUCT_DICTIONARY: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 2; plan.dst = ip0 + 2 + code[plan.argc] * 2; } break;
		case OPCODE_CONSTRUCT_TYPED_ARRAY: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.elem_script = ip0 + 3 + code[plan.argc] * 1; plan.elem_builtin = ip0 + 5 + code[plan.argc] * 1; plan.elem_native = ip0 + 6 + code[plan.argc] * 1; } break;
		case OPCODE_CONSTRUCT_TYPED_DICTIONARY: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 2; plan.dst = ip0 + 2 + code[plan.argc] * 2; plan.key_script = ip0 + 3 + code[plan.argc] * 2; plan.key_builtin = ip0 + 6 + code[plan.argc] * 2; plan.key_native = ip0 + 7 + code[plan.argc] * 2; plan.value_script = ip0 + 4 + code[plan.argc] * 2; plan.value_builtin = ip0 + 8 + code[plan.argc] * 2; plan.value_native = ip0 + 9 + code[plan.argc] * 2; } break;
		case OPCODE_CONSTRUCT_VALIDATED: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 4 + code[plan.argc]; } break;
		case OPCODE_CREATE_LAMBDA: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		case OPCODE_CREATE_SELF_LAMBDA: { plan.argc = ip0 + 2 + code[ip0 + 1]; plan.args = ip0 + 2; plan.args_step = 1; plan.dst = ip0 + 2 + code[plan.argc] * 1; plan.payload = ip0 + 3 + code[ip0 + 1]; } break;
		default: break;
	}
	return plan;
}

} // namespace gdscript_disassemble
} // namespace godot
