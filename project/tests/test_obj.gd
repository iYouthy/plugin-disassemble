extends SceneTree
func _initialize() -> void:
	print(GDScriptBytecodeDumper.new().disassemble(load("res://tests/objects.gd")))
	quit(0)
