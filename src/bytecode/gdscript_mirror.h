// Reading GDScript's compiled form out of the running engine.
//
// Godot exposes no API for GDScript bytecode: `GDScript` binds only `new()`,
// `ScriptLanguage` binds nothing at all, and there is no debugger message for
// it. The compiled `GDScriptFunction` objects are reachable only from inside
// the engine, and `GDScriptFunction` is not even a ClassDB class.
//
// So we go around the API: take the raw engine pointer that godot-cpp keeps for
// every wrapped object, and read the engine's own struct layout at the offsets
// measured by tools/gen_layout.py. This is deliberately narrow -- we only ever
// read, never write -- and every entry point validates before it trusts.
//
// Three things make this defensible rather than reckless:
//
//   1. The offsets are *measured* from the engine headers with the engine's own
//      compile flags (tools/gen_layout.py), never hand-written.
//   2. godot-cpp's Variant/String/StringName/HashMap/Vector/... are byte-identical
//      copies of the engine's own containers, so reinterpreting engine memory as
//      a godot-cpp type is sound. The static_asserts below enforce the parts of
//      that agreement we depend on, at compile time.
//   3. `verify()` cross-checks what we found against ground truth that IS public
//      (Script::get_script_method_list), so a build whose layout differs
//      degrades to an error message instead of reading garbage.

#pragma once

#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "engine_layout.h"
#include "opcodes.h"

namespace godot {
namespace gdscript_disassemble {

// ---------------------------------------------------------------------------
// Compile-time ABI guards
// ---------------------------------------------------------------------------
// These compare godot-cpp's compiled sizes against the sizes measured inside
// the engine. If a future godot-cpp changes any of them, the build fails here
// rather than misreading engine memory at runtime.

static_assert(sizeof(Variant) == kLayout.kSize_Variant,
		"godot-cpp Variant no longer matches the engine layout table");
static_assert(sizeof(StringName) == kLayout.kSize_StringName,
		"godot-cpp StringName no longer matches the engine layout table");
static_assert(sizeof(String) == kLayout.kSize_String,
		"godot-cpp String no longer matches the engine layout table");
static_assert(sizeof(Vector<int>) == kLayout.kSize_Vector_int,
		"godot-cpp Vector no longer matches the engine layout table");
static_assert(sizeof(HashMap<int, int>) == kLayout.kSize_HashMap_int_int,
		"godot-cpp HashMap no longer matches the engine layout table");
static_assert(sizeof(List<int>) == kLayout.kSize_List_int,
		"godot-cpp List no longer matches the engine layout table");

// Stand-in for an engine-internal value type we iterate past but never read.
// Only the size matters: the container's element stride has to come out right.
template <size_t N>
struct Opaque {
	unsigned char bytes[N];
};

using LambdaInfoMirror = Opaque<kLayout.kSize_GDScript_LambdaInfo>;
using MemberInfoMirror = Opaque<kLayout.kSize_GDScript_MemberInfo>;

// ---------------------------------------------------------------------------
// Raw engine pointers
// ---------------------------------------------------------------------------

// godot-cpp stores the engine-side object for every wrapper it hands out. The
// field is public (with a "do not touch" comment); reading it is the one honest
// way to get from a Ref<Script> to the engine's GDScript*.
inline const void *engine_pointer(const Object *p_wrapper) {
	return p_wrapper == nullptr ? nullptr : static_cast<const void *>(p_wrapper->_owner);
}

// Views one engine struct through the generated offset table.
class RawView {
public:
	RawView() = default;
	explicit RawView(const void *p_base) :
			_base(static_cast<const char *>(p_base)) {}

	bool valid() const { return _base != nullptr; }
	const void *base() const { return _base; }

	template <typename T>
	const T &at(size_t p_offset) const {
		return *reinterpret_cast<const T *>(_base + p_offset);
	}

private:
	const char *_base = nullptr;
};

// ---------------------------------------------------------------------------
// GDScriptFunction
// ---------------------------------------------------------------------------

// A read-only view of one engine GDScriptFunction.
class FunctionView : public RawView {
public:
	FunctionView() = default;
	using RawView::RawView;

	StringName get_name() const { return at<StringName>(kLayout.kOff_GDScriptFunction_name); }
	// A static function compiles to the same instructions as an instance one when
	// neither touches `self`, so this is the only way to tell them apart.
	bool is_static() const { return at<bool>(kLayout.kOff_GDScriptFunction__static); }
	const void *get_script() const { return at<const void *>(kLayout.kOff_GDScriptFunction__script); }
	int get_initial_line() const { return at<int>(kLayout.kOff_GDScriptFunction__initial_line); }
	int get_argument_count() const { return at<int>(kLayout.kOff_GDScriptFunction__argument_count); }
	bool is_vararg() const { return at<int>(kLayout.kOff_GDScriptFunction__vararg_index) >= 0; }

	// The instruction stream. `_code_ptr` aliases `code.write[0]`, so either
	// would do; the pointer is cheaper and is what the engine's own disassembler
	// walks.
	const int *code() const { return at<const int *>(kLayout.kOff_GDScriptFunction__code_ptr); }
	int code_size() const { return at<int>(kLayout.kOff_GDScriptFunction__code_size); }

	const Variant *constants() const { return at<const Variant *>(kLayout.kOff_GDScriptFunction__constants_ptr); }
	int constant_count() const { return at<int>(kLayout.kOff_GDScriptFunction__constant_count); }

	const StringName *global_names() const { return at<const StringName *>(kLayout.kOff_GDScriptFunction__global_names_ptr); }
	int global_name_count() const { return at<int>(kLayout.kOff_GDScriptFunction__global_names_count); }

	const void *const *methods() const { return at<const void *const *>(kLayout.kOff_GDScriptFunction__methods_ptr); }
	const void *const *lambdas() const { return at<const void *const *>(kLayout.kOff_GDScriptFunction__lambdas_ptr); }

	// Counts for the two operand tables that have no separate `_*_count` field.
	// Without these an out-of-range operand index would index a wild pointer --
	// which is not a theoretical concern: it is what crashed the editor.
	int method_count() const { return at<const Vector<const void *>>(kLayout.kOff_GDScriptFunction_methods).size(); }
	int lambda_count() const { return at<const Vector<const void *>>(kLayout.kOff_GDScriptFunction_lambdas).size(); }

	Variant get_constant(int p_index) const {
		if (p_index < 0 || p_index >= constant_count()) {
			return Variant();
		}
		return constants()[p_index];
	}

	StringName get_global_name(int p_index) const {
		if (p_index < 0 || p_index >= global_name_count()) {
			return StringName();
		}
		return global_names()[p_index];
	}

	// DEBUG_ENABLED-only tables mapping an operand index to the name the engine
	// itself would print. Looking names up here keeps the listing in step with
	// the engine instead of with a hand-copied enum.
	String debug_name(size_t p_table_offset, int p_index) const {
		const Vector<String> &table = at<const Vector<String>>(p_table_offset);
		if (p_index < 0 || p_index >= table.size()) {
			return String();
		}
		return table[p_index];
	}

	// MethodBind::name sits at a measured offset; the bind object itself is an
	// engine class godot-cpp does not model, so we only read its name.
	StringName get_method_name(int p_index) const {
		const void *const *m = methods();
		if (m == nullptr || p_index < 0 || p_index >= method_count()) {
			return StringName();
		}
		const void *bind = m[p_index];
		if (bind == nullptr) {
			return StringName();
		}
		return *reinterpret_cast<const StringName *>(static_cast<const char *>(bind) + kLayout.kOff_MethodBind_name);
	}

	// A cheap structural sanity check: walking the instruction stream must land
	// exactly on code_size() with OPCODE_END last. If the layout table were
	// wrong this would almost certainly fail, so it gates the disassembler.
	bool looks_like_valid_code() const;
};

// ---------------------------------------------------------------------------
// GDScript
// ---------------------------------------------------------------------------

class ScriptMirror : public RawView {
public:
	ScriptMirror() = default;
	using RawView::RawView;

	// Builds a mirror for `p_script` if it really is a GDScript of a shape this
	// build understands. On failure returns false and fills r_error with
	// something worth showing the user.
	static bool bind(Script *p_script, ScriptMirror &r_out, String &r_error);

	// member_functions: every compiled method of this class, including the
	// synthesised "@implicit_initializer" / "@implicit_ready" entries.
	const HashMap<StringName, void *> &member_functions() const {
		return at<const HashMap<StringName, void *>>(kLayout.kOff_GDScript_member_functions);
	}

	// Lambdas are not in member_functions; they live in their own map.
	const HashMap<void *, LambdaInfoMirror> &lambda_info() const {
		return at<const HashMap<void *, LambdaInfoMirror>>(kLayout.kOff_GDScript_lambda_info);
	}

	const void *implicit_initializer() const { return at<const void *>(kLayout.kOff_GDScript_implicit_initializer); }
	const void *implicit_ready() const { return at<const void *>(kLayout.kOff_GDScript_implicit_ready); }
	const void *static_initializer() const { return at<const void *>(kLayout.kOff_GDScript_static_initializer); }

	const HashMap<StringName, MemberInfoMirror> &member_indices() const {
		return at<const HashMap<StringName, MemberInfoMirror>>(kLayout.kOff_GDScript_member_indices);
	}

	const HashMap<StringName, Variant> &constants() const {
		return at<const HashMap<StringName, Variant>>(kLayout.kOff_GDScript_constants);
	}

	// Nested `class Inner:` blocks. The engine's value type is `Ref<GDScript>`,
	// and a Ref holds a pointer to the *engine* object while godot-cpp's Ref
	// holds a pointer to its own wrapper -- so this is read as an opaque engine
	// pointer and recursed into directly. It is never handed to godot-cpp as a
	// Ref, which would misinterpret it.
	const HashMap<StringName, const void *> &subclasses() const {
		return at<const HashMap<StringName, const void *>>(kLayout.kOff_GDScript_subclasses);
	}

	// The engine resolves a member index (an operand of ADDR_TYPE_MEMBER) by
	// scanning member_indices for the entry with that index.
	String member_name_by_index(int p_index) const;

	// Maximum nesting depth walked. A malformed layout could otherwise turn the
	// `subclasses` recursion into a cycle.
	static constexpr int kMaxClassDepth = 16;

	// Every function reachable from this class, in a stable order, annotated
	// with where it came from. Nested classes are walked depth-first.
	struct Entry {
		StringName name;
		String path; // "Outer.Inner.method", including nesting, for display
		FunctionView function;
		int depth = 0;
		// The class this function was compiled into. Member indices are resolved
		// against the *owning* class, so a nested class's members are not found
		// in the outer script's table.
		const void *owner = nullptr;
	};
	void collect_functions(List<Entry> &r_out, const String &p_prefix = String(), int p_depth = 0) const;

	// Cross-checks the mirror against public ground truth: the keys of
	// member_functions must be exactly the method names Script reports.
	// This is what turns a wrong layout into a message instead of garbage.
	bool verify(Script *p_script, String &r_error) const;
};

} // namespace gdscript_disassemble
} // namespace godot
