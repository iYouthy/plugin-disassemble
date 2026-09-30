// GENERATED FILE -- do not edit by hand.
// Regenerate with: python3 tools/gen_instruction_sizes.py --godot <engine-source>
//
// Instruction lengths, extracted from the advance (`incr`) the engine's own
// disassembler applies to each opcode. Decoding the stream without this is
// impossible -- instructions carry no length of their own -- and getting one
// wrong desynchronises everything after it.

#pragma once

#include "opcodes.h"

namespace godot {
namespace gdscript_disassemble {

// sizeof(Variant::ValidatedOperatorEvaluator) / sizeof(int) -- 2 on 64-bit.
inline constexpr int kPointerSize = (int)(sizeof(void *) / sizeof(int));

// Length in ints of the instruction starting at p_ip, or 0 if the opcode is
// not one this table knows (which callers must treat as a decode failure).
// `code` is the whole instruction stream and `ip0` the opcode's slot, matching
// the naming the engine's own disassembler uses for the same expression.
inline int instruction_length(int p_opcode, const int *code, int ip0) {
	switch (p_opcode) {
		case OPCODE_OPERATOR: return 7 + kPointerSize;
		case OPCODE_OPERATOR_VALIDATED: return 5;
		case OPCODE_TYPE_TEST_BUILTIN: return 4;
		case OPCODE_TYPE_TEST_ARRAY: return 6;
		case OPCODE_TYPE_TEST_DICTIONARY: return 9;
		case OPCODE_TYPE_TEST_NATIVE: return 4;
		case OPCODE_TYPE_TEST_SCRIPT: return 4;
		case OPCODE_SET_KEYED: return 4;
		case OPCODE_SET_KEYED_VALIDATED: return 5;
		case OPCODE_SET_INDEXED_VALIDATED: return 5;
		case OPCODE_GET_KEYED: return 4;
		case OPCODE_GET_KEYED_VALIDATED: return 5;
		case OPCODE_GET_INDEXED_VALIDATED: return 5;
		case OPCODE_SET_NAMED: return 4;
		case OPCODE_SET_NAMED_VALIDATED: return 4;
		case OPCODE_GET_NAMED: return 4;
		case OPCODE_GET_NAMED_VALIDATED: return 4;
		case OPCODE_SET_MEMBER: return 3;
		case OPCODE_GET_MEMBER: return 3;
		case OPCODE_SET_STATIC_VARIABLE: return 4;
		case OPCODE_GET_STATIC_VARIABLE: return 4;
		case OPCODE_ASSIGN: return 3;
		case OPCODE_ASSIGN_NULL: return 2;
		case OPCODE_ASSIGN_TRUE: return 2;
		case OPCODE_ASSIGN_FALSE: return 2;
		case OPCODE_ASSIGN_TYPED_BUILTIN: return 4;
		case OPCODE_ASSIGN_TYPED_ARRAY: return 6;
		case OPCODE_ASSIGN_TYPED_DICTIONARY: return 9;
		case OPCODE_ASSIGN_TYPED_NATIVE: return 4;
		case OPCODE_ASSIGN_TYPED_SCRIPT: return 4;
		case OPCODE_CAST_TO_BUILTIN: return 4;
		case OPCODE_CAST_TO_NATIVE: return 4;
		case OPCODE_CAST_TO_SCRIPT: return 4;
		case OPCODE_CONSTRUCT: return 1 + 3 + (code[ip0 + 1]);
		case OPCODE_CONSTRUCT_VALIDATED: return 1 + 3 + (code[ip0 + 1]);
		case OPCODE_CONSTRUCT_ARRAY: return 1 + 3 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CONSTRUCT_TYPED_ARRAY: return 1 + 6 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CONSTRUCT_DICTIONARY: return 1 + 3 + (code[ip0 + 2 + code[ip0 + 1]]) * 2;
		case OPCODE_CONSTRUCT_TYPED_DICTIONARY: return 1 + 9 + (code[ip0 + 2 + code[ip0 + 1]]) * 2;
		case OPCODE_CALL: return 1 + 5 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_RETURN: return 1 + 5 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_ASYNC: return 1 + 5 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_UTILITY: return 1 + 4 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_UTILITY_VALIDATED: return 1 + 4 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_GDSCRIPT_UTILITY: return 1 + 4 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_BUILTIN_TYPE_VALIDATED: return 1 + 5 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_SELF_BASE: return 1 + 4 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_METHOD_BIND: return 1 + 5 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_METHOD_BIND_RET: return 1 + 5 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_BUILTIN_STATIC: return 1 + 5 + (code[ip0 + 4 + code[ip0 + 1]]);
		case OPCODE_CALL_NATIVE_STATIC: return 1 + 4 + (code[ip0 + 3 + code[ip0 + 1]]);
		case OPCODE_CALL_NATIVE_STATIC_VALIDATED_RETURN: return 1 + 4 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_NATIVE_STATIC_VALIDATED_NO_RETURN: return 1 + 4 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_METHOD_BIND_VALIDATED_RETURN: return 1 + 5 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CALL_METHOD_BIND_VALIDATED_NO_RETURN: return 1 + 5 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_AWAIT: return 2;
		case OPCODE_AWAIT_RESUME: return 2;
		case OPCODE_CREATE_LAMBDA: return 1 + 4 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_CREATE_SELF_LAMBDA: return 1 + 4 + (code[ip0 + 2 + code[ip0 + 1]]);
		case OPCODE_JUMP: return 2;
		case OPCODE_JUMP_IF: return 3;
		case OPCODE_JUMP_IF_NOT: return 3;
		case OPCODE_JUMP_TO_DEF_ARGUMENT: return 1;
		case OPCODE_JUMP_IF_SHARED: return 3;
		case OPCODE_RETURN: return 2;
		case OPCODE_RETURN_TYPED_BUILTIN: return 3;
		case OPCODE_RETURN_TYPED_ARRAY: return 5;
		case OPCODE_RETURN_TYPED_DICTIONARY: return 8;
		case OPCODE_RETURN_TYPED_NATIVE: return 3;
		case OPCODE_RETURN_TYPED_SCRIPT: return 3;
		case OPCODE_ITERATE_BEGIN: return 5;
		case OPCODE_ITERATE_BEGIN_INT: return 5;
		case OPCODE_ITERATE_BEGIN_FLOAT: return 5;
		case OPCODE_ITERATE_BEGIN_VECTOR2: return 5;
		case OPCODE_ITERATE_BEGIN_VECTOR2I: return 5;
		case OPCODE_ITERATE_BEGIN_VECTOR3: return 5;
		case OPCODE_ITERATE_BEGIN_VECTOR3I: return 5;
		case OPCODE_ITERATE_BEGIN_STRING: return 5;
		case OPCODE_ITERATE_BEGIN_DICTIONARY: return 5;
		case OPCODE_ITERATE_BEGIN_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_BYTE_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_INT32_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_INT64_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_FLOAT32_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_FLOAT64_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_STRING_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_VECTOR2_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_VECTOR3_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_COLOR_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_PACKED_VECTOR4_ARRAY: return 5;
		case OPCODE_ITERATE_BEGIN_OBJECT: return 5;
		case OPCODE_ITERATE_BEGIN_RANGE: return 7;
		case OPCODE_ITERATE: return 5;
		case OPCODE_ITERATE_INT: return 5;
		case OPCODE_ITERATE_FLOAT: return 5;
		case OPCODE_ITERATE_VECTOR2: return 5;
		case OPCODE_ITERATE_VECTOR2I: return 5;
		case OPCODE_ITERATE_VECTOR3: return 5;
		case OPCODE_ITERATE_VECTOR3I: return 5;
		case OPCODE_ITERATE_STRING: return 5;
		case OPCODE_ITERATE_DICTIONARY: return 5;
		case OPCODE_ITERATE_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_BYTE_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_INT32_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_INT64_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_FLOAT32_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_FLOAT64_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_STRING_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_VECTOR2_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_VECTOR3_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_COLOR_ARRAY: return 5;
		case OPCODE_ITERATE_PACKED_VECTOR4_ARRAY: return 5;
		case OPCODE_ITERATE_OBJECT: return 5;
		case OPCODE_ITERATE_RANGE: return 6;
		case OPCODE_STORE_GLOBAL: return 3;
		case OPCODE_STORE_NAMED_GLOBAL: return 3;
		case OPCODE_TYPE_ADJUST_BOOL: return 2;
		case OPCODE_TYPE_ADJUST_INT: return 2;
		case OPCODE_TYPE_ADJUST_FLOAT: return 2;
		case OPCODE_TYPE_ADJUST_STRING: return 2;
		case OPCODE_TYPE_ADJUST_VECTOR2: return 2;
		case OPCODE_TYPE_ADJUST_VECTOR2I: return 2;
		case OPCODE_TYPE_ADJUST_RECT2: return 2;
		case OPCODE_TYPE_ADJUST_RECT2I: return 2;
		case OPCODE_TYPE_ADJUST_VECTOR3: return 2;
		case OPCODE_TYPE_ADJUST_VECTOR3I: return 2;
		case OPCODE_TYPE_ADJUST_TRANSFORM2D: return 2;
		case OPCODE_TYPE_ADJUST_VECTOR4: return 2;
		case OPCODE_TYPE_ADJUST_VECTOR4I: return 2;
		case OPCODE_TYPE_ADJUST_PLANE: return 2;
		case OPCODE_TYPE_ADJUST_QUATERNION: return 2;
		case OPCODE_TYPE_ADJUST_AABB: return 2;
		case OPCODE_TYPE_ADJUST_BASIS: return 2;
		case OPCODE_TYPE_ADJUST_TRANSFORM3D: return 2;
		case OPCODE_TYPE_ADJUST_PROJECTION: return 2;
		case OPCODE_TYPE_ADJUST_COLOR: return 2;
		case OPCODE_TYPE_ADJUST_STRING_NAME: return 2;
		case OPCODE_TYPE_ADJUST_NODE_PATH: return 2;
		case OPCODE_TYPE_ADJUST_RID: return 2;
		case OPCODE_TYPE_ADJUST_OBJECT: return 2;
		case OPCODE_TYPE_ADJUST_CALLABLE: return 2;
		case OPCODE_TYPE_ADJUST_SIGNAL: return 2;
		case OPCODE_TYPE_ADJUST_DICTIONARY: return 2;
		case OPCODE_TYPE_ADJUST_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_BYTE_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_INT32_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_INT64_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_FLOAT32_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_FLOAT64_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_STRING_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_VECTOR2_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_VECTOR3_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_COLOR_ARRAY: return 2;
		case OPCODE_TYPE_ADJUST_PACKED_VECTOR4_ARRAY: return 2;
		case OPCODE_ASSERT: return 3;
		case OPCODE_BREAKPOINT: return 1;
		case OPCODE_LINE: return 2;
		case OPCODE_END: return 1;
		default: return 0;
	}
}

} // namespace gdscript_disassemble
} // namespace godot
