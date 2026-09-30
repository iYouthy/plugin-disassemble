// The read-only bytecode pane that sits beside the code editor.
//
// It is a plain Control so it can be dropped into the script editor's
// HSplitContainer and take part in the same drag-to-resize behaviour the file
// list and the code editor already use.

#pragma once

#include <godot_cpp/classes/code_edit.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include "bytecode_highlighter.h"

namespace godot {

class BytecodePanel : public VBoxContainer {
	GDCLASS(BytecodePanel, VBoxContainer)

protected:
	static void _bind_methods();

public:
	BytecodePanel();
	~BytecodePanel() override;

	// Replaces the pane's contents. A non-empty `p_error` switches to the error
	// presentation -- that is how a script which fails to compile is reported
	// rather than showing a stale or empty listing.
	void show_listing(const String &p_listing, const String &p_error);
	void show_message(const String &p_message);

	void set_status(const String &p_status);

	// Called when the user hits Refresh. Set by the plugin.
	void set_refresh_callback(const Callable &p_callback);

private:
	void _on_refresh_pressed();

	Label *_status = nullptr;
	CodeEdit *_view = nullptr;
	Ref<BytecodeHighlighter> _highlighter;
	Callable _refresh_callback;
};

} // namespace godot
