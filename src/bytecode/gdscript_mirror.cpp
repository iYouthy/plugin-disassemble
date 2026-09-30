#include "gdscript_mirror.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
namespace gdscript_disassemble {

bool FunctionView::looks_like_valid_code() const {
	if (!valid()) {
		return false;
	}
	const int *c = code();
	const int n = code_size();
	if (c == nullptr || n < 1 || n > (1 << 24)) {
		return false;
	}
	// Every compiled function is terminated by OPCODE_END, and the first slot is
	// always an opcode. A wrong `_code_ptr`/`_code_size` offset almost never
	// satisfies both.
	if (c[n - 1] != OPCODE_END) {
		return false;
	}
	if (c[0] < 0 || c[0] >= kOpcodeCount) {
		return false;
	}
	const int cc = constant_count();
	if (cc < 0 || (cc > 0 && constants() == nullptr)) {
		return false;
	}
	const int gc = global_name_count();
	if (gc < 0 || (gc > 0 && global_names() == nullptr)) {
		return false;
	}
	return true;
}

String ScriptMirror::member_name_by_index(int p_index) const {
	const HashMap<StringName, MemberInfoMirror> &members = member_indices();
	for (const KeyValue<StringName, MemberInfoMirror> &E : members) {
		const char *entry = reinterpret_cast<const char *>(&E.value);
		const int index = *reinterpret_cast<const int *>(entry + kLayout.kOff_GDScript_MemberInfo_index);
		if (index == p_index) {
			return String(E.key);
		}
	}
	return String();
}

void ScriptMirror::collect_functions(List<Entry> &r_out, const String &p_prefix, int p_depth) const {
	if (!valid()) {
		return;
	}

	auto add = [&](const StringName &p_name, const void *p_function) {
		if (p_function == nullptr) {
			return;
		}
		Entry e;
		e.name = p_name;
		e.function = FunctionView(p_function);
		e.depth = p_depth;
		e.owner = base();
		e.path = p_prefix.is_empty() ? String(p_name) : p_prefix + String(".") + String(p_name);
		r_out.push_back(e);
	};

	// The order the engine's own test harness uses: synthesised initialisers
	// first, then lambdas, then ordinary methods.
	add(StringName("@implicit_initializer"), implicit_initializer());
	add(StringName("@implicit_ready"), implicit_ready());
	add(StringName("@static_initializer"), static_initializer());

	for (const KeyValue<void *, LambdaInfoMirror> &E : lambda_info()) {
		// Lambdas are keyed by their function pointer and conventionally named
		// by the engine as "lambda@<line>"; the name is read off the function.
		// The structural check matters here: reading a name out of something that
		// is not a compiled function copies a StringName from arbitrary memory,
		// which aborts the editor rather than failing gracefully.
		const FunctionView f(E.key);
		add(f.looks_like_valid_code() ? f.get_name() : StringName("lambda"), E.key);
	}

	for (const KeyValue<StringName, void *> &E : member_functions()) {
		add(E.key, E.value);
	}

	// Nested `class Inner:` blocks are separate GDScript objects, each with its
	// own compiled functions. Without this their bodies simply do not appear.
	if (p_depth < kMaxClassDepth) {
		for (const KeyValue<StringName, const void *> &E : subclasses()) {
			if (E.value == nullptr) {
				continue;
			}
			const String child_prefix = p_prefix.is_empty()
					? String(E.key)
					: p_prefix + String(".") + String(E.key);
			ScriptMirror(E.value).collect_functions(r_out, child_prefix, p_depth + 1);
		}
	}
}

bool ScriptMirror::verify(Script *p_script, String &r_error) const {
	if (!valid()) {
		r_error = "Not a GDScript object.";
		return false;
	}

	// 1. Which engine this build has been verified against.
	//
	// The layout is compiled in, so it describes one engine build -- but a new
	// engine release does not necessarily change it. Measured: 4.7.2 and 4.8-dev
	// agree on all 62 constants. Refusing on the version *string* would therefore
	// reject engines the table describes perfectly well, which is the opposite of
	// what is wanted here: one build, usable across releases.
	//
	// So the string decides only the floor. Anything at or above the verified
	// version is allowed through and the structural checks below -- which read
	// real engine state and compare against real public API -- are what actually
	// decide whether the table fits.
	Engine *engine = Engine::get_singleton();
	if (engine != nullptr) {
		const Dictionary v = engine->get_version_info();
		const int major = (int)v.get("major", 0);
		const int minor = (int)v.get("minor", 0);
		const int patch = (int)v.get("patch", 0);

		// Bump these when a layout is regenerated and verified for a new engine.
		constexpr int kFloorMajor = 4;
		constexpr int kFloorMinor = 7;
		constexpr int kFloorPatch = 2;

		const bool below_floor = (major < kFloorMajor) ||
				(major == kFloorMajor && (minor < kFloorMinor ||
												 (minor == kFloorMinor && patch < kFloorPatch)));
		if (below_floor) {
			r_error = vformat(
					"This build's bytecode layout is for Godot %d.%d.%d or newer, but this editor is "
					"Godot %d.%d.%d. Regenerate src/bytecode/engine_layout.h for it:\n"
					"    python3 tools/regen.py --godot <engine-source>",
					kFloorMajor, kFloorMinor, kFloorPatch, major, minor, patch);
			return false;
		}

		const String running = vformat("%d.%d.%d", major, minor, patch);
		if (running != String(kLayoutEngineVersion)) {
			// Newer than anything verified. Allowed, but said out loud: if the
			// checks below then fail, this line is what explains why.
			WARN_PRINT(vformat("GDScript Disassemble: layout was verified against Godot %s, running on %s. "
							   "Proceeding because the structural checks below pass; if they fail, "
							   "regenerate the layout tables for this engine.",
					String(kLayoutEngineVersion), running));
		}
	}

	// 2. Self-consistency: every map key must equal the function's own name.
	// This validates the map offset, the iteration, and the `name` offset at
	// once -- three independent things would have to be wrong in a matching way
	// for a broken layout to slip through.
	const HashMap<StringName, void *> &functions = member_functions();
	int checked = 0;
	for (const KeyValue<StringName, void *> &E : functions) {
		if (E.value == nullptr) {
			continue;
		}
		FunctionView f(E.value);
		if (!f.valid()) {
			continue;
		}
		if (f.get_name() != E.key) {
			r_error = "GDScript memory layout does not match this engine build "
						  "(function name mismatch). Regenerate the layout table.";
			return false;
		}
		checked++;
	}

	// 3. Cross-check against public ground truth. `get_script_method_list` is a
	// real bound method, so its contents are independent of our offsets.
	TypedArray<Dictionary> reported = p_script->get_script_method_list();
	if (!reported.is_empty()) {
		bool found_any = false;
		for (const Variant &v : reported) {
			const Dictionary d = v;
			const StringName name = d.get("name", StringName());
			if (functions.has(name)) {
				found_any = true;
				break;
			}
		}
		if (!found_any) {
			r_error = "GDScript memory layout does not match this engine build "
						  "(no method names matched). Regenerate the layout table.";
			return false;
		}
	} else if (checked == 0 && !functions.is_empty()) {
		r_error = "GDScript memory layout does not match this engine build.";
		return false;
	}

	return true;
}

bool ScriptMirror::bind(Script *p_script, ScriptMirror &r_out, String &r_error) {
	if (p_script == nullptr) {
		r_error = "No script selected.";
		return false;
	}
	const void *raw = engine_pointer(p_script);
	if (raw == nullptr) {
		r_error = "Could not reach the engine object for this script.";
		return false;
	}
	r_out = ScriptMirror(raw);
	return r_out.verify(p_script, r_error);
}

} // namespace gdscript_disassemble
} // namespace godot
