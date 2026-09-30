#include "disassemble_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "bytecode/disassembler.h"

using namespace godot::gdscript_disassemble;

namespace godot {

void GDScriptDisassemblePlugin::_bind_methods() {}

GDScriptDisassemblePlugin::GDScriptDisassemblePlugin() {}

GDScriptDisassemblePlugin::~GDScriptDisassemblePlugin() {}

// The script editor's layout container holds both the toolbar row and the split
// that contains the code editor. It is found by *shape* rather than by a fixed
// depth, because that shape is what survives a restructure:
//
//   4.7  ScriptEditor (PanelContainer) -> VBoxContainer -> [HBox(menu), HSplit]
//   4.8  ScriptEditor (EditorDock) -> DocumentEditorContainer -> VBoxContainer -> same
//
// The same VBox simply moved one level down when the script editor was rebuilt
// on EditorDock, and menu_hb / script_split moved with it out of ScriptEditor
// into DocumentEditorContainer. Matching on the shape covers both without a
// version check.
static VBoxContainer *find_layout_box(Node *p_root, int p_depth) {
	if (p_root == nullptr || p_depth > 4) {
		return nullptr;
	}
	for (int i = 0; i < p_root->get_child_count(); i++) {
		Node *child = p_root->get_child(i);
		VBoxContainer *box = Object::cast_to<VBoxContainer>(child);
		if (box != nullptr) {
			bool has_toolbar = false;
			bool has_split = false;
			for (int j = 0; j < box->get_child_count(); j++) {
				if (Object::cast_to<HBoxContainer>(box->get_child(j)) != nullptr) {
					has_toolbar = true;
				}
				if (Object::cast_to<HSplitContainer>(box->get_child(j)) != nullptr) {
					has_split = true;
				}
			}
			if (has_toolbar && has_split) {
				return box;
			}
		}
		if (VBoxContainer *found = find_layout_box(child, p_depth + 1)) {
			return found;
		}
	}
	return nullptr;
}

// Printed when the layout cannot be found, so a future restructure is
// diagnosable from the console instead of looking like the plugin silently
// doing nothing.
static void print_node_tree(Node *p_node, int p_depth) {
	if (p_node == nullptr || p_depth > 5) {
		return;
	}
	String indent;
	for (int i = 0; i < p_depth; i++) {
		indent += String("  ");
	}
	UtilityFunctions::print(indent, p_node->get_class(), " \"", p_node->get_name(), "\"");
	for (int i = 0; i < p_node->get_child_count(); i++) {
		print_node_tree(p_node->get_child(i), p_depth + 1);
	}
}

Control *GDScriptDisassemblePlugin::_find_child_of_layout(bool p_want_split) const {
	VBoxContainer *box = find_layout_box(_script_editor, 0);
	if (box == nullptr) {
		return nullptr;
	}
	for (int j = 0; j < box->get_child_count(); j++) {
		Node *child = box->get_child(j);
		if (p_want_split ? (Object::cast_to<HSplitContainer>(child) != nullptr)
						 : (Object::cast_to<HBoxContainer>(child) != nullptr)) {
			return Object::cast_to<Control>(child);
		}
	}
	return nullptr;
}

Control *GDScriptDisassemblePlugin::_find_menu_bar() const {
	return _find_child_of_layout(false);
}

Control *GDScriptDisassemblePlugin::_find_script_split() const {
	return _find_child_of_layout(true);
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
		// Print the actual tree rather than just giving up: when the editor
		// restructures this layout (as 4.8 does), this output is what makes the
		// difference between a five-minute fix and guessing.
		WARN_PRINT("GDScript Disassemble: could not find the script editor layout; the plugin will do nothing. "
				   "The tree it searched follows.");
		print_node_tree(_script_editor, 0);
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
