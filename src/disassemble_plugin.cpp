#include "disassemble_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include "bytecode/disassembler.h"

using namespace godot::gdscript_disassemble;

namespace godot {

void GDScriptDisassemblePlugin::_bind_methods() {}

GDScriptDisassemblePlugin::GDScriptDisassemblePlugin() {}

GDScriptDisassemblePlugin::~GDScriptDisassemblePlugin() {}

// The Script Editor is a PanelContainer holding a VBoxContainer ("main
// container"), whose first child is the horizontal menu bar and whose second is
// the HSplitContainer that splits [file list | code editor]. Both are private
// C++ members with no accessor, so they are found by shape. Looking children up
// by type rather than by index keeps this working when the editor inserts
// another button into the menu bar, which it already does lazily for the
// Edit/Go To menus.
Control *GDScriptDisassemblePlugin::_find_menu_bar() const {
	if (_script_editor == nullptr) {
		return nullptr;
	}
	for (int i = 0; i < _script_editor->get_child_count(); i++) {
		VBoxContainer *main_container = Object::cast_to<VBoxContainer>(_script_editor->get_child(i));
		if (main_container == nullptr) {
			continue;
		}
		for (int j = 0; j < main_container->get_child_count(); j++) {
			if (Object::cast_to<HBoxContainer>(main_container->get_child(j)) != nullptr) {
				return Object::cast_to<Control>(main_container->get_child(j));
			}
		}
	}
	return nullptr;
}

Control *GDScriptDisassemblePlugin::_find_script_split() const {
	if (_script_editor == nullptr) {
		return nullptr;
	}
	for (int i = 0; i < _script_editor->get_child_count(); i++) {
		VBoxContainer *main_container = Object::cast_to<VBoxContainer>(_script_editor->get_child(i));
		if (main_container == nullptr) {
			continue;
		}
		for (int j = 0; j < main_container->get_child_count(); j++) {
			if (Object::cast_to<HSplitContainer>(main_container->get_child(j)) != nullptr) {
				return Object::cast_to<Control>(main_container->get_child(j));
			}
		}
	}
	return nullptr;
}

void GDScriptDisassemblePlugin::_enter_tree() {
	EditorInterface *ei = EditorInterface::get_singleton();
	if (ei == nullptr) {
		return;
	}
	_script_editor = ei->get_script_editor();
	if (_script_editor == nullptr) {
		WARN_PRINT("GDScript Disassemble: could not find the Script Editor; the plugin will do nothing.");
		return;
	}

	Control *menu_bar = _find_menu_bar();
	if (menu_bar == nullptr) {
		WARN_PRINT("GDScript Disassemble: could not find the script editor menu bar; the plugin will do nothing.");
		return;
	}

	_toggle = memnew(Button);
	_toggle->set_text("Bytecode");
	_toggle->set_toggle_mode(true);
	_toggle->set_theme_type_variation("FlatButton");
	_toggle->set_tooltip_text("Show the compiled bytecode of the current script beside the editor");
	_toggle->connect("toggled", callable_mp(this, &GDScriptDisassemblePlugin::_on_toggle_pressed));
	menu_bar->add_child(_toggle);

	_panel = memnew(BytecodePanel);
	_panel->set_refresh_callback(callable_mp(this, &GDScriptDisassemblePlugin::_refresh));
	_panel->hide();

	// "editor_script_changed" is emitted on tab switches and when a script is
	// opened; the engine itself never connects to it, so it is safe to use.
	_script_editor->connect("editor_script_changed", callable_mp(this, &GDScriptDisassemblePlugin::_on_script_changed));

	if (_script_editor->has_signal("script_close")) {
		_script_editor->connect("script_close", callable_mp(this, &GDScriptDisassemblePlugin::_on_script_changed));
	}
}

void GDScriptDisassemblePlugin::_exit_tree() {
	if (_script_editor != nullptr) {
		if (_script_editor->is_connected("editor_script_changed", callable_mp(this, &GDScriptDisassemblePlugin::_on_script_changed))) {
			_script_editor->disconnect("editor_script_changed", callable_mp(this, &GDScriptDisassemblePlugin::_on_script_changed));
		}
	}
	if (_panel != nullptr) {
		if (_panel->get_parent() != nullptr) {
			_panel->get_parent()->remove_child(_panel);
		}
		_panel->queue_free();
		_panel = nullptr;
	}
	if (_toggle != nullptr) {
		if (_toggle->get_parent() != nullptr) {
			_toggle->get_parent()->remove_child(_toggle);
		}
		_toggle->queue_free();
		_toggle = nullptr;
	}
	_script_editor = nullptr;
}

void GDScriptDisassemblePlugin::_on_toggle_pressed(bool p_pressed) {
	_enabled = p_pressed;
	Control *split = _find_script_split();
	if (_panel == nullptr || split == nullptr) {
		return;
	}

	if (p_pressed) {
		if (_panel->get_parent() == nullptr) {
			// Third pane of the script editor's own split: [files | code | bytecode].
			// The user drags the same divider the editor already provides.
			split->add_child(_panel);
		}
		_panel->show();
		_refresh();
	} else {
		_panel->hide();
	}
}

void GDScriptDisassemblePlugin::_on_script_changed(const Ref<Script> &p_script) {
	_on_tab_changed();
}

void GDScriptDisassemblePlugin::_on_tab_changed() {
	if (_enabled) {
		_refresh();
	}
}

void GDScriptDisassemblePlugin::_refresh() {
	if (_panel == nullptr || _script_editor == nullptr) {
		return;
	}

	Ref<Script> script = _script_editor->get_current_script();
	if (script.is_null()) {
		_panel->show_message("No script is open.");
		return;
	}

	if (!script->has_source_code()) {
		_panel->show_message("The current editor tab is not a text script.");
		return;
	}

	// Anything that is not a GDScript (C#, visual scripts, ...) has no VM
	// bytecode to show. Say so rather than reporting a layout mismatch.
	if (!script->is_class("GDScript")) {
		_panel->show_message(vformat("Bytecode view supports GDScript only (this is %s).",
				script->get_class()));
		return;
	}

	// Parse/analysis failures surface here as an error string; the pane shows
	// them in place of a listing.
	String text;
	String error;
	if (!disassemble_script(script.ptr(), text, error)) {
		_panel->show_listing(String(), error);
		return;
	}
	_panel->show_listing(text, String());
}

} // namespace godot
