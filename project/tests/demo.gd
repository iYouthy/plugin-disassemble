extends SceneTree
func _initialize() -> void:
	print(GDScriptBytecodeDumper.new().disassemble(load("res://tests/objdemo.gd")))
	quit(0)
