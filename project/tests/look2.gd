extends SceneTree
func _initialize() -> void:
	print(GDScriptBytecodeDumper.new().disassemble(load("res://tests/statics.gd")))
	quit(0)
