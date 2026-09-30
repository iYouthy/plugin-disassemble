extends SceneTree
func _initialize() -> void:
	print(GDScriptBytecodeDumper.new().disassemble(load("res://tests/nested.gd")))
	quit(0)
