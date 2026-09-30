extends SceneTree
func _initialize() -> void:
	var d := GDScriptBytecodeDumper.new()
	var s: Script = load("res://tests/example.gd")
	print(d.disassemble(s))
	quit(0)
