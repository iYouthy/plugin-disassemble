extends SceneTree
func _initialize() -> void:
	var step := 0
	for a in OS.get_cmdline_user_args():
		step = int(a)
	print("=== step ", step, " ===")
	var r := GDScriptBytecodeDumper.new().probe_object_names(load("res://tests/objects.gd"), step)
	print("=== done: ", r, " ===")
	quit(0)
