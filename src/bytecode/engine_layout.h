// GENERATED FILE -- do not edit by hand.
// Regenerate with: python3 tools/gen_layout.py --godot <engine-source>
//
// Member offsets of the engine's GDScript / GDScriptFunction, as measured from
// the engine headers themselves with the engine's own compile flags.
//
// Engine version : 4.7.2-stable
// Build flags    : -DDEBUG_ENABLED -DNDEBUG -DTOOLS_ENABLED
// Layout id      : 5922183044973855246
//
// Offsets depend on the engine version AND on which #ifdefs (TOOLS_ENABLED,
// DEBUG_ENABLED, ...) were active. `verify_layout()` cross-checks the entries
// marked [anchor] against the running engine before any bytecode is read, so a
// mismatched build degrades to a clear message instead of reading garbage.

#pragma once

#include <cstddef>

namespace godot {
namespace gdscript_disassemble {

// Identifies the engine build this table was generated from.
inline constexpr const char *kLayoutEngineVersion = "4.7.2";
inline constexpr const char *kLayoutBuildFlags = "-DDEBUG_ENABLED -DNDEBUG -DTOOLS_ENABLED";
inline constexpr unsigned long long kLayoutId = 5922183044973855246ULL;

struct EngineLayout {
	size_t kSize_Object = 336;  // sizeof(Object)
	size_t kSize_RefCounted = 352;  // sizeof(RefCounted)
	size_t kSize_Resource = 480;  // sizeof(Resource)
	size_t kSize_Script = 480;  // sizeof(Script)
	size_t kSize_GDScript = 1728;  // sizeof(GDScript)
	size_t kSize_GDScriptFunction = 1152;  // sizeof(GDScriptFunction)
	size_t kOff_GDScript_member_functions = 688;  // offsetof(GDScript, member_functions)
	size_t kOff_GDScript_constants = 648;  // offsetof(GDScript, constants)
	size_t kOff_GDScript_subclasses = 728;  // offsetof(GDScript, subclasses)
	size_t kOff_GDScript_member_indices = 512;  // offsetof(GDScript, member_indices)
	size_t kOff_GDScript_members = 552;  // offsetof(GDScript, members)
	size_t kOff_GDScript_static_variables = 632;  // offsetof(GDScript, static_variables)
	size_t kOff_GDScript_static_variables_indices = 592;  // offsetof(GDScript, static_variables_indices)
	size_t kOff_GDScript_lambda_info = 816;  // offsetof(GDScript, lambda_info)
	size_t kOff_GDScript_implicit_initializer = 1480;  // offsetof(GDScript, implicit_initializer)
	size_t kOff_GDScript_implicit_ready = 1488;  // offsetof(GDScript, implicit_ready)
	size_t kOff_GDScript_static_initializer = 1496;  // offsetof(GDScript, static_initializer)
	size_t kOff_GDScript_valid = 481;  // offsetof(GDScript, valid)
	size_t kOff_GDScript_base = 496;  // offsetof(GDScript, base)
	size_t kOff_GDScript_local_name = 1568;  // offsetof(GDScript, local_name)
	size_t kOff_GDScript_fully_qualified_name = 1584;  // offsetof(GDScript, fully_qualified_name)
	size_t kOff_GDScriptNativeClass_name = 352;  // offsetof(GDScriptNativeClass, name)
	size_t kOff_GDScript_native = 488;  // offsetof(GDScript, native)
	size_t kOff_GDScriptFunction_name = 0;  // offsetof(GDScriptFunction, name)
	size_t kOff_GDScriptFunction_method_info = 88;  // offsetof(GDScriptFunction, method_info)
	size_t kOff_GDScriptFunction__script = 232;  // offsetof(GDScriptFunction, _script)
	size_t kOff_GDScriptFunction__initial_line = 240;  // offsetof(GDScriptFunction, _initial_line)
	size_t kOff_GDScriptFunction__argument_count = 244;  // offsetof(GDScriptFunction, _argument_count)
	size_t kOff_GDScriptFunction__vararg_index = 248;  // offsetof(GDScriptFunction, _vararg_index)
	size_t kOff_GDScriptFunction__stack_size = 252;  // offsetof(GDScriptFunction, _stack_size)
	size_t kOff_GDScriptFunction_code = 344;  // offsetof(GDScriptFunction, code)
	size_t kOff_GDScriptFunction_constants = 376;  // offsetof(GDScriptFunction, constants)
	size_t kOff_GDScriptFunction_global_names = 432;  // offsetof(GDScriptFunction, global_names)
	size_t kOff_GDScriptFunction_methods = 624;  // offsetof(GDScriptFunction, methods)
	size_t kOff_GDScriptFunction_lambdas = 640;  // offsetof(GDScriptFunction, lambdas)
	size_t kOff_GDScriptFunction__code_size = 656;  // offsetof(GDScriptFunction, _code_size)
	size_t kOff_GDScriptFunction__constant_count = 664;  // offsetof(GDScriptFunction, _constant_count)
	size_t kOff_GDScriptFunction__global_names_count = 668;  // offsetof(GDScriptFunction, _global_names_count)
	size_t kOff_GDScriptFunction__code_ptr = 728;  // offsetof(GDScriptFunction, _code_ptr)
	size_t kOff_GDScriptFunction__constants_ptr = 744;  // offsetof(GDScriptFunction, _constants_ptr)
	size_t kOff_GDScriptFunction__global_names_ptr = 752;  // offsetof(GDScriptFunction, _global_names_ptr)
	size_t kOff_GDScriptFunction__methods_ptr = 848;  // offsetof(GDScriptFunction, _methods_ptr)
	size_t kOff_GDScriptFunction__lambdas_ptr = 856;  // offsetof(GDScriptFunction, _lambdas_ptr)
	size_t kOff_GDScriptFunction_operator_names = 880;  // offsetof(GDScriptFunction, operator_names)
	size_t kOff_GDScriptFunction_setter_names = 896;  // offsetof(GDScriptFunction, setter_names)
	size_t kOff_GDScriptFunction_getter_names = 912;  // offsetof(GDScriptFunction, getter_names)
	size_t kOff_GDScriptFunction_builtin_methods_names = 928;  // offsetof(GDScriptFunction, builtin_methods_names)
	size_t kOff_GDScriptFunction_constructors_names = 944;  // offsetof(GDScriptFunction, constructors_names)
	size_t kOff_GDScriptFunction_utilities_names = 960;  // offsetof(GDScriptFunction, utilities_names)
	size_t kOff_GDScriptFunction_gds_utilities_names = 976;  // offsetof(GDScriptFunction, gds_utilities_names)
	size_t kOff_MethodBind_name = 16;  // offsetof(MethodBind, name)
	size_t kOff_MethodBind_argument_count = 52;  // offsetof(MethodBind, argument_count)
	size_t kSize_GDScript_LambdaInfo = 8;  // sizeof(GDScript::LambdaInfo)
	size_t kSize_GDScript_MemberInfo = 120;  // sizeof(GDScript::MemberInfo)
	size_t kOff_GDScript_MemberInfo_index = 0;  // offsetof(GDScript::MemberInfo, index)
	size_t kSize_Variant = 24;  // sizeof(Variant)  [anchor]
	size_t kSize_StringName = 8;  // sizeof(StringName)  [anchor]
	size_t kSize_String = 8;  // sizeof(String)  [anchor]
	size_t kSize_Vector_int = 16;  // sizeof(Vector<int>)  [anchor]
	size_t kSize_HashMap_int_int = 40;  // sizeof(HashMap<int, int>)  [anchor]
	size_t kSize_List_int = 8;  // sizeof(List<int>)  [anchor]
};

// The table below is the active layout.
inline constexpr EngineLayout kLayout = {
	.kSize_Object = 336,
	.kSize_RefCounted = 352,
	.kSize_Resource = 480,
	.kSize_Script = 480,
	.kSize_GDScript = 1728,
	.kSize_GDScriptFunction = 1152,
	.kOff_GDScript_member_functions = 688,
	.kOff_GDScript_constants = 648,
	.kOff_GDScript_subclasses = 728,
	.kOff_GDScript_member_indices = 512,
	.kOff_GDScript_members = 552,
	.kOff_GDScript_static_variables = 632,
	.kOff_GDScript_static_variables_indices = 592,
	.kOff_GDScript_lambda_info = 816,
	.kOff_GDScript_implicit_initializer = 1480,
	.kOff_GDScript_implicit_ready = 1488,
	.kOff_GDScript_static_initializer = 1496,
	.kOff_GDScript_valid = 481,
	.kOff_GDScript_base = 496,
	.kOff_GDScript_local_name = 1568,
	.kOff_GDScript_fully_qualified_name = 1584,
	.kOff_GDScriptNativeClass_name = 352,
	.kOff_GDScript_native = 488,
	.kOff_GDScriptFunction_name = 0,
	.kOff_GDScriptFunction_method_info = 88,
	.kOff_GDScriptFunction__script = 232,
	.kOff_GDScriptFunction__initial_line = 240,
	.kOff_GDScriptFunction__argument_count = 244,
	.kOff_GDScriptFunction__vararg_index = 248,
	.kOff_GDScriptFunction__stack_size = 252,
	.kOff_GDScriptFunction_code = 344,
	.kOff_GDScriptFunction_constants = 376,
	.kOff_GDScriptFunction_global_names = 432,
	.kOff_GDScriptFunction_methods = 624,
	.kOff_GDScriptFunction_lambdas = 640,
	.kOff_GDScriptFunction__code_size = 656,
	.kOff_GDScriptFunction__constant_count = 664,
	.kOff_GDScriptFunction__global_names_count = 668,
	.kOff_GDScriptFunction__code_ptr = 728,
	.kOff_GDScriptFunction__constants_ptr = 744,
	.kOff_GDScriptFunction__global_names_ptr = 752,
	.kOff_GDScriptFunction__methods_ptr = 848,
	.kOff_GDScriptFunction__lambdas_ptr = 856,
	.kOff_GDScriptFunction_operator_names = 880,
	.kOff_GDScriptFunction_setter_names = 896,
	.kOff_GDScriptFunction_getter_names = 912,
	.kOff_GDScriptFunction_builtin_methods_names = 928,
	.kOff_GDScriptFunction_constructors_names = 944,
	.kOff_GDScriptFunction_utilities_names = 960,
	.kOff_GDScriptFunction_gds_utilities_names = 976,
	.kOff_MethodBind_name = 16,
	.kOff_MethodBind_argument_count = 52,
	.kSize_GDScript_LambdaInfo = 8,
	.kSize_GDScript_MemberInfo = 120,
	.kOff_GDScript_MemberInfo_index = 0,
	.kSize_Variant = 24,
	.kSize_StringName = 8,
	.kSize_String = 8,
	.kSize_Vector_int = 16,
	.kSize_HashMap_int_int = 40,
	.kSize_List_int = 8,
};

} // namespace gdscript_disassemble
} // namespace godot
