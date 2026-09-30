extends SceneTree
func _initialize() -> void:
	print(GDScriptBytecodeDumper.new().disassemble(load("res://tests/constructs.gd")))
	quit(0)
