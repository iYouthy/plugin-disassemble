// Syntax highlighting for the bytecode listing.
//
// The listing is not GDScript, but it is read like it: identifiers, numbers,
// strings, and calls. So the palette is taken from the editor theme's own
// GDScript colours rather than hard-coded, which keeps the pane looking native
// and makes it follow the user's light/dark theme for free.
//
// A plain SyntaxHighlighter (rather than CodeHighlighter) is used because the
// listing has structure a keyword list cannot express: two fixed-width numeric
// columns that should recede, and address operands (`stack(3)`, `member(speed)`)
// that deserve a colour of their own.

#pragma once

#include <godot_cpp/classes/syntax_highlighter.hpp>

namespace godot {

class BytecodeHighlighter : public SyntaxHighlighter {
	GDCLASS(BytecodeHighlighter, SyntaxHighlighter)

protected:
	static void _bind_methods();

public:
	BytecodeHighlighter();

	// Re-reads the palette from the editor theme. Call before rendering so a
	// theme change is picked up.
	void refresh_colors();

	Dictionary _get_line_syntax_highlighting(int32_t p_line) const override;

	// The tokeniser, split out from the virtual so it is a pure function of the
	// line text. That makes the colouring testable without a rendering editor --
	// the virtual itself can only be driven by the engine's drawing path.
	Dictionary highlight_line(const String &p_line) const;

private:
	// Width of the "%6s %6s  " prefix the renderer emits: source line, bytecode
	// offset, then the expression.
	static constexpr int kPrefixWidth = 15;

	void _add(Dictionary &r_out, int p_column, const Color &p_color) const;

	Color _keyword;
	Color _address;
	Color _function;
	Color _number;
	Color _string;
	Color _symbol;
	Color _plain;
	Color _muted;
	Color _heading;
};

} // namespace godot
