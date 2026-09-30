#include "bytecode_panel.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/h_separator.hpp>

namespace godot {

void BytecodePanel::_bind_methods() {}

BytecodePanel::BytecodePanel() {
	set_name("BytecodePanel");
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);

	// Header: title on the left, refresh on the right, matching the flat
	// icon-button look the editor uses elsewhere.
	HBoxContainer *header = memnew(HBoxContainer);
	add_child(header);

	Label *title = memnew(Label);
	title->set_text("Bytecode");
	title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	header->add_child(title);

	Button *refresh = memnew(Button);
	refresh->set_text("Refresh");
	refresh->set_theme_type_variation("FlatButton");
	refresh->set_tooltip_text("Recompile and disassemble the current script");
	refresh->connect("pressed", callable_mp(this, &BytecodePanel::_on_refresh_pressed));
	header->add_child(refresh);

	add_child(memnew(HSeparator));

	// The listing itself. CodeEdit rather than TextEdit so the editor's own code
	// font, gutter, and selection behaviour apply, and so a highlighter can be
	// attached later without changing the widget.
	_view = memnew(CodeEdit);
	_view->set_editable(false);
	_view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	_view->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_view->set_draw_line_numbers(false);
	_view->set_draw_fold_gutter(false);
	_view->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_NONE);
	// Bytecode is read like code, so highlight it like code -- using the editor
	// theme's own GDScript palette.
	_highlighter.instantiate();
	_view->set_syntax_highlighter(_highlighter);
	add_child(_view);

	_status = memnew(Label);
	_status->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	_status->add_theme_color_override("font_color",
			EditorInterface::get_singleton()->get_editor_theme()->get_color("error_color", "Editor"));
	_status->hide();
	add_child(_status);
}

BytecodePanel::~BytecodePanel() {}

void BytecodePanel::_on_refresh_pressed() {
	if (_refresh_callback.is_valid()) {
		_refresh_callback.call();
	}
}

void BytecodePanel::set_refresh_callback(const Callable &p_callback) {
	_refresh_callback = p_callback;
}

void BytecodePanel::show_listing(const String &p_listing, const String &p_error) {
	if (!p_error.is_empty()) {
		show_message(p_error);
		return;
	}
	_status->hide();
	_highlighter->refresh_colors();
	_view->set_text(p_listing);
	_view->set_editable(false);
}

void BytecodePanel::show_message(const String &p_message) {
	_view->set_text("");
	_status->set_text(p_message);
	_status->show();
}

void BytecodePanel::set_status(const String &p_status) {
	if (p_status.is_empty()) {
		_status->hide();
		return;
	}
	_status->set_text(p_status);
	_status->show();
}

} // namespace godot
