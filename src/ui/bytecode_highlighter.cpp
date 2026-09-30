#include "bytecode_highlighter.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Address operands: the VM's storage kinds. Coloured distinctly because seeing
// at a glance whether an operand is a stack slot, a constant, or a member is the
// main thing one reads a listing for.
static const char *kAddressWords[] = { "stack", "const", "member", "self", "class", "nil" };

// Structural words emitted by the renderer.
static const char *kKeywordWords[] = {
	"line", "jump", "if", "not", "return", "end", "breakpoint", "assert",
	"for-init", "for-loop", "for", "in", "counter", "step", "range",
	"await", "as", "is", "static", "global", "named_global", "object",
	"utility", "ctor", "bind", "lambda", "type"
};

static bool _is_word_char(char32_t c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
			c == '_' || c == '@' || c == '#';
}

static bool _in_list(const String &p_word, const char **p_list, int p_count) {
	for (int i = 0; i < p_count; i++) {
		if (p_word == p_list[i]) {
			return true;
		}
	}
	return false;
}

void BytecodeHighlighter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("highlight_line", "line"), &BytecodeHighlighter::highlight_line);
}

BytecodeHighlighter::BytecodeHighlighter() {
	refresh_colors();
}

void BytecodeHighlighter::refresh_colors() {
	// Defaults chosen to stay legible if the theme lookup fails; the editor
	// theme normally supplies all of these.
	_keyword = Color(0.94f, 0.44f, 0.60f);
	_address = Color(0.42f, 0.78f, 0.94f);
	_function = Color(0.55f, 0.78f, 0.42f);
	_number = Color(0.68f, 0.79f, 0.53f);
	_string = Color(0.96f, 0.71f, 0.44f);
	_symbol = Color(0.80f, 0.80f, 0.80f);
	_plain = Color(0.85f, 0.87f, 0.90f);
	_muted = Color(0.48f, 0.50f, 0.55f);
	_heading = Color(0.62f, 0.68f, 0.80f);

	// EditorInterface only exists inside the editor; get_singleton() errors
	// loudly elsewhere (e.g. a headless tool run loading this class).
	if (Engine::get_singleton() == nullptr || !Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	EditorInterface *ei = EditorInterface::get_singleton();
	if (ei == nullptr) {
		return;
	}
	const Ref<Theme> theme = ei->get_editor_theme();
	if (theme.is_null()) {
		return;
	}

	struct Mapping {
		const char *name;
		Color *target;
	};
	const Mapping mappings[] = {
		{ "keyword_color", &_keyword },
		{ "base_type_color", &_address },
		{ "function_color", &_function },
		{ "number_color", &_number },
		{ "string_color", &_string },
		{ "symbol_color", &_symbol },
		{ "member_variable_color", &_plain },
		{ "comment_color", &_muted },
		{ "accent_color", &_heading },
	};
	for (const Mapping &m : mappings) {
		if (theme->has_color(m.name, "Editor")) {
			*m.target = theme->get_color(m.name, "Editor");
		}
	}
}

void BytecodeHighlighter::_add(Dictionary &r_out, int p_column, const Color &p_color) const {
	Dictionary entry;
	entry["color"] = p_color;
	r_out[p_column] = entry;
}

Dictionary BytecodeHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
	TextEdit *edit = get_text_edit();
	if (edit == nullptr || p_line < 0 || p_line >= edit->get_line_count()) {
		return Dictionary();
	}
	return highlight_line(edit->get_line(p_line));
}

Dictionary BytecodeHighlighter::highlight_line(const String &p_line) const {
	Dictionary out;
	const String text = p_line;
	const int length = text.length();

	// Header and separator lines ("Script: ...", "--- foo ---", "=====") have no
	// numeric prefix; render them as a single muted heading.
	if (length < kPrefixWidth || text[6] != ' ' || text[13] != ' ' || text[14] != ' ') {
		_add(out, 0, text.begins_with("---") ? _heading : _muted);
		return out;
	}

	// The two numeric columns are reference data, not the thing being read, so
	// they recede -- the same way line numbers do in the code editor.
	_add(out, 0, _muted);
	_add(out, kPrefixWidth, _plain);

	// Every branch below must move `i` forward. Going through this helper makes
	// that structural rather than a property of each branch: a scanner that
	// returns an end position which is not past the start would otherwise spin
	// forever, and this runs on the main thread inside the editor's drawing
	// path -- so the failure mode is a frozen editor, not a bad colour.
	int i = kPrefixWidth;
	auto advance = [&i](int p_next) {
		i = (p_next > i) ? p_next : i + 1;
	};

	while (i < length) {
		const char32_t c = text[i];

		if (c == ' ') {
			i++;
			continue;
		}

		// String literal.
		if (c == '"') {
			int j = i + 1;
			while (j < length && text[j] != '"') {
				j++;
			}
			_add(out, i, _string);
			advance((j < length) ? j + 1 : j);
			continue;
		}

		// `<anonymous lambda>` and friends: a bracketed name, which may contain
		// spaces, so it cannot be scanned as an ordinary identifier.
		if (c == '<') {
			int j = i + 1;
			while (j < length && text[j] != '>') {
				j++;
			}
			_add(out, i, _function);
			advance((j < length) ? j + 1 : j);
			continue;
		}

		// Number, including a leading '-' as in `const(-1.0)`.
		if ((c >= '0' && c <= '9') || (c == '-' && i + 1 < length && text[i + 1] >= '0' && text[i + 1] <= '9')) {
			int j = i;
			// The sign is what lets this branch be entered, but it is not part of
			// the digit run the scan accepts -- so consume it explicitly.
			if (text[j] == '-') {
				j++;
			}
			while (j < length && ((text[j] >= '0' && text[j] <= '9') || text[j] == '.')) {
				j++;
			}
			_add(out, i, _number);
			advance(j);
			continue;
		}

		// Identifier or keyword.
		if (_is_word_char(c)) {
			int j = i;
			while (j < length && _is_word_char(text[j])) {
				j++;
			}
			const String word = text.substr(i, j - i);

			// A following '(' makes it a call: constructors (`float(`), methods,
			// engine functions (`print(`), and the pseudo-calls `stack(`/`const(`.
			int k = j;
			while (k < length && text[k] == ' ') {
				k++;
			}
			const bool is_call = (k < length && text[k] == '(');

			if (_in_list(word, kAddressWords, 6)) {
				_add(out, i, _address);
			} else if (_in_list(word, kKeywordWords, (int)(sizeof(kKeywordWords) / sizeof(kKeywordWords[0])))) {
				_add(out, i, _keyword);
			} else if (is_call) {
				_add(out, i, _function);
			} else {
				_add(out, i, _plain);
			}
			advance(j);
			continue;
		}

		// Anything else is punctuation.
		_add(out, i, _symbol);
		i++;
	}

	return out;
}

} // namespace godot
