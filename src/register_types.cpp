
#include <gdextension_interface.h>
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "bytecode/bytecode_dumper.h"
#include "disassemble_plugin.h"
#include "ui/bytecode_highlighter.h"
#include "ui/bytecode_panel.h"

using namespace godot;

static void initialize_gdscript_disassemble(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// The pane is a plain Control; registering it at SCENE level keeps it
		// constructible anywhere, including in the editor.
		GDREGISTER_CLASS(BytecodePanel);
		GDREGISTER_CLASS(BytecodeHighlighter);
		GDREGISTER_CLASS(GDScriptBytecodeDumper);
	}

	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_CLASS(GDScriptDisassemblePlugin);
		EditorPlugins::add_by_type<GDScriptDisassemblePlugin>();
	}
}

static void uninitialize_gdscript_disassemble(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorPlugins::remove_by_type<GDScriptDisassemblePlugin>();
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT gdscript_disassemble_library_init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_gdscript_disassemble);
	init_obj.register_terminator(uninitialize_gdscript_disassemble);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
