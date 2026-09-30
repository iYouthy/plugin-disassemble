// EditorPlugin that adds a "View Bytecode" toggle to the script editor and
// opens a read-only disassembly pane beside the code.

#pragma once

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/script_editor.hpp>

#include "ui/bytecode_panel.h"

namespace godot {

class GDScriptDisassemblePlugin : public EditorPlugin {
	GDCLASS(GDScriptDisassemblePlugin, EditorPlugin)

protected:
	static void _bind_methods();

public:
	GDScriptDisassemblePlugin();
	~GDScriptDisassemblePlugin() override;

	void _enter_tree() override;
	void _exit_tree() override;

private:
	void _on_toggle_pressed(bool p_pressed);
	void _on_script_changed(const Ref<Script> &p_script);
	void _on_tab_changed();
	void _refresh();

	// The Script Editor owns `menu_hb` and `script_split` as private members, so
	// they are found structurally. Returns null if the layout is not the one this
	// plugin knows how to extend.
	Control *_find_menu_bar() const;
	Control *_find_script_split() const;

	bool _enabled = false;
	Button *_toggle = nullptr;
	BytecodePanel *_panel = nullptr;
	ScriptEditor *_script_editor = nullptr;
};

} // namespace godot
