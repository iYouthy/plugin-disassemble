// Scriptable entry point to the disassembler.
//
// The panel uses this, and so can any tool script -- which also makes the whole
// read-the-engine's-memory path testable without a GUI.

#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/script.hpp>

namespace godot {

class GDScriptBytecodeDumper : public RefCounted {
	GDCLASS(GDScriptBytecodeDumper, RefCounted)

protected:
	static void _bind_methods();

public:
	// Returns the disassembly of `p_script`, or an explanation of why it could
	// not be produced (wrong script type, engine layout mismatch, undecodable
	// stream). Never returns an empty string without saying why.
	String disassemble(const Ref<Script> &p_script) const;

	// True when the compiled layout matches the engine this build was generated
	// for. Call this to distinguish "no bytecode" from "cannot read bytecode".
	bool is_layout_supported() const;

	String get_layout_info() const;
};

} // namespace godot
