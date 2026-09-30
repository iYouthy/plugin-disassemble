extends SceneTree

func _initialize() -> void:
	var dumper := GDScriptBytecodeDumper.new()
	print("layout_supported=", dumper.is_layout_supported())
	print("info=", dumper.get_layout_info())
	var script: Script = load("res://tests/example.gd")
	if script == null:
		print("FAILED to load script")
		quit(1)
		return
	print(dumper.disassemble(script))
	quit(0)
