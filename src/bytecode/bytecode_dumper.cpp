#include "bytecode_dumper.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "disassembler.h"

namespace godot {

void GDScriptBytecodeDumper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("disassemble", "script"), &GDScriptBytecodeDumper::disassemble);
	ClassDB::bind_method(D_METHOD("is_layout_supported"), &GDScriptBytecodeDumper::is_layout_supported);
	ClassDB::bind_method(D_METHOD("get_layout_info"), &GDScriptBytecodeDumper::get_layout_info);
	ClassDB::bind_method(D_METHOD("self_test", "script"), &GDScriptBytecodeDumper::self_test);
	ClassDB::bind_method(D_METHOD("probe_object_names", "script", "step"), &GDScriptBytecodeDumper::probe_object_names);
}

bool GDScriptBytecodeDumper::is_layout_supported() const {
	Engine *engine = Engine::get_singleton();
	if (engine == nullptr) {
		return false;
	}
	const Dictionary v = engine->get_version_info();
	return vformat("%d.%d.%d", (int)v.get("major", 0), (int)v.get("minor", 0), (int)v.get("patch", 0)) ==
			String(gdscript_disassemble::kLayoutEngineVersion);
}

String GDScriptBytecodeDumper::self_test(const Ref<Script> &p_script) const {
	if (p_script.is_null() || !p_script->is_class("GDScript")) {
		return String("only GDScript is supported");
	}
	return gdscript_disassemble::check_fixed_operands(p_script.ptr());
}

String GDScriptBytecodeDumper::probe_object_names(const Ref<Script> &p_script, int32_t p_step) const {
	return gdscript_disassemble::probe_object_names(p_script.ptr(), p_step);
}

String GDScriptBytecodeDumper::get_layout_info() const {
	return vformat("layout generated for Godot %s, built with [%s]",
			gdscript_disassemble::kLayoutEngineVersion,
			gdscript_disassemble::kLayoutBuildFlags);
}

String GDScriptBytecodeDumper::disassemble(const Ref<Script> &p_script) const {
	if (p_script.is_null()) {
		return "No script given.";
	}
	if (!p_script->is_class("GDScript")) {
		return vformat("Only GDScript is supported (this is %s).", p_script->get_class());
	}

	String text;
	String error;
	if (!gdscript_disassemble::disassemble_script(p_script.ptr(), text, error)) {
		return error;
	}
	return text;
}

} // namespace godot
