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

// Operands of the fixed-shape instructions, split by kind. Positions are
// in the order the engine reads them, so the order *is* the engine's, not a
// restatement of it.
//
// The kind matters as much as the position: an address operand and a raw int
// are read through the same slot but mean different things, and reading one as
// the other yields a plausible-looking wrong value.
struct FixedPlan {
	int addr[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	int addr_count = 0;
	int imm[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	int imm_count = 0;
};

inline FixedPlan fixed_plan(int p_opcode, int ip0) {
	FixedPlan plan;
	switch (p_opcode) {
		case OPCODE_ASSERT: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm_count = 0; } break;
		case OPCODE_ASSIGN: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm_count = 0; } break;
		case OPCODE_ASSIGN_FALSE: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm_count = 0; } break;
		case OPCODE_ASSIGN_NULL: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm_count = 0; } break;
		case OPCODE_ASSIGN_TRUE: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm_count = 0; } break;
		case OPCODE_ASSIGN_TYPED_ARRAY: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm_count = 0; } break;
		case OPCODE_ASSIGN_TYPED_BUILTIN: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm_count = 1; } break;
		case OPCODE_ASSIGN_TYPED_DICTIONARY: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm_count = 0; } break;
		case OPCODE_ASSIGN_TYPED_NATIVE: { plan.addr[0] = ip0 + 3; plan.addr[1] = ip0 + 1; plan.addr[2] = ip0 + 2; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_ASSIGN_TYPED_SCRIPT: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm_count = 1; } break;
		case OPCODE_AWAIT: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm_count = 0; } break;
		case OPCODE_AWAIT_RESUME: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm_count = 0; } break;
		case OPCODE_CAST_TO_BUILTIN: { plan.addr[0] = ip0 + 2; plan.addr[1] = ip0 + 1; plan.addr_count = 2; plan.imm[0] = ip0 + 1; plan.imm_count = 1; } break;
		case OPCODE_CAST_TO_NATIVE: { plan.addr[0] = ip0 + 2; plan.addr[1] = ip0 + 1; plan.addr[2] = ip0 + 3; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_CAST_TO_SCRIPT: { plan.addr[0] = ip0 + 2; plan.addr[1] = ip0 + 1; plan.addr[2] = ip0 + 3; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_GET_INDEXED_VALIDATED: { plan.addr[0] = ip0 + 3; plan.addr[1] = ip0 + 1; plan.addr[2] = ip0 + 2; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_GET_KEYED: { plan.addr[0] = ip0 + 3; plan.addr[1] = ip0 + 1; plan.addr[2] = ip0 + 2; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_GET_KEYED_VALIDATED: { plan.addr[0] = ip0 + 3; plan.addr[1] = ip0 + 1; plan.addr[2] = ip0 + 2; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_GET_MEMBER: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm_count = 1; } break;
		case OPCODE_GET_NAMED: { plan.addr[0] = ip0 + 2; plan.addr[1] = ip0 + 1; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm_count = 1; } break;
		case OPCODE_GET_NAMED_VALIDATED: { plan.addr[0] = ip0 + 2; plan.addr[1] = ip0 + 1; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm_count = 1; } break;
		case OPCODE_GET_STATIC_VARIABLE: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm[1] = ip0 + 3; plan.imm_count = 2; } break;
		case OPCODE_ITERATE: { plan.addr[0] = ip0 + 3; plan.addr[1] = ip0 + 2; plan.addr[2] = ip0 + 1; plan.addr_count = 3; plan.imm[0] = ip0 + 4; plan.imm_count = 1; } break;
		case OPCODE_ITERATE_BEGIN: { plan.addr[0] = ip0 + 3; plan.addr[1] = ip0 + 2; plan.addr[2] = ip0 + 1; plan.addr_count = 3; plan.imm[0] = ip0 + 4; plan.imm_count = 1; } break;
		case OPCODE_ITERATE_BEGIN_RANGE: { plan.addr[0] = ip0 + 5; plan.addr[1] = ip0 + 2; plan.addr[2] = ip0 + 3; plan.addr[3] = ip0 + 4; plan.addr[4] = ip0 + 1; plan.addr_count = 5; plan.imm[0] = ip0 + 6; plan.imm_count = 1; } break;
		case OPCODE_ITERATE_RANGE: { plan.addr[0] = ip0 + 4; plan.addr[1] = ip0 + 2; plan.addr[2] = ip0 + 3; plan.addr[3] = ip0 + 1; plan.addr_count = 4; plan.imm[0] = ip0 + 5; plan.imm_count = 1; } break;
		case OPCODE_JUMP: { plan.addr_count = 0; plan.imm[0] = ip0 + 1; plan.imm_count = 1; } break;
		case OPCODE_JUMP_IF: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm_count = 1; } break;
		case OPCODE_JUMP_IF_NOT: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm_count = 1; } break;
		case OPCODE_JUMP_IF_SHARED: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm_count = 1; } break;
		case OPCODE_LINE: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 1; plan.imm_count = 1; } break;
		case OPCODE_OPERATOR: { plan.addr[0] = ip0 + 3; plan.addr[1] = ip0 + 1; plan.addr[2] = ip0 + 2; plan.addr_count = 3; plan.imm[0] = ip0 + 4; plan.imm_count = 1; } break;
		case OPCODE_OPERATOR_VALIDATED: { plan.addr[0] = ip0 + 3; plan.addr[1] = ip0 + 1; plan.addr[2] = ip0 + 2; plan.addr_count = 3; plan.imm[0] = ip0 + 4; plan.imm_count = 1; } break;
		case OPCODE_RETURN: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm_count = 0; } break;
		case OPCODE_RETURN_TYPED_ARRAY: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm_count = 0; } break;
		case OPCODE_RETURN_TYPED_BUILTIN: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm_count = 1; } break;
		case OPCODE_RETURN_TYPED_DICTIONARY: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm_count = 0; } break;
		case OPCODE_RETURN_TYPED_NATIVE: { plan.addr[0] = ip0 + 2; plan.addr[1] = ip0 + 1; plan.addr_count = 2; plan.imm_count = 0; } break;
		case OPCODE_RETURN_TYPED_SCRIPT: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 3; plan.addr[2] = ip0 + 2; plan.addr_count = 3; plan.imm[0] = ip0 + 2; plan.imm[1] = ip0 + 4; plan.imm_count = 2; } break;
		case OPCODE_SET_INDEXED_VALIDATED: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr[2] = ip0 + 3; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_SET_KEYED: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr[2] = ip0 + 3; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_SET_KEYED_VALIDATED: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr[2] = ip0 + 3; plan.addr_count = 3; plan.imm_count = 0; } break;
		case OPCODE_SET_MEMBER: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm_count = 1; } break;
		case OPCODE_SET_NAMED: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm_count = 1; } break;
		case OPCODE_SET_NAMED_VALIDATED: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm_count = 1; } break;
		case OPCODE_SET_STATIC_VARIABLE: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm[1] = ip0 + 3; plan.imm_count = 2; } break;
		case OPCODE_STORE_GLOBAL: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm_count = 1; } break;
		case OPCODE_STORE_NAMED_GLOBAL: { plan.addr[0] = ip0 + 1; plan.addr_count = 1; plan.imm[0] = ip0 + 2; plan.imm_count = 1; } break;
		case OPCODE_TYPE_TEST_ARRAY: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm[1] = ip0 + 4; plan.imm[2] = ip0 + 5; plan.imm_count = 3; } break;
		case OPCODE_TYPE_TEST_BUILTIN: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm_count = 1; } break;
		case OPCODE_TYPE_TEST_DICTIONARY: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm[1] = ip0 + 5; plan.imm[2] = ip0 + 6; plan.imm[3] = ip0 + 4; plan.imm[4] = ip0 + 7; plan.imm[5] = ip0 + 8; plan.imm_count = 6; } break;
		case OPCODE_TYPE_TEST_NATIVE: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr_count = 2; plan.imm[0] = ip0 + 3; plan.imm_count = 1; } break;
		case OPCODE_TYPE_TEST_SCRIPT: { plan.addr[0] = ip0 + 1; plan.addr[1] = ip0 + 2; plan.addr[2] = ip0 + 3; plan.addr_count = 3; plan.imm_count = 0; } break;
		default: break;
	}
	return plan;
}

} // namespace gdscript_disassemble
} // namespace godot
