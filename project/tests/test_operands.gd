extends SceneTree
func _initialize() -> void:
	var d := GDScriptBytecodeDumper.new()
	var total := 0
	for name in ["example", "constructs", "nested", "objects", "tool_script", "empty"]:
		var r: String = d.self_test(load("res://tests/%s.gd" % name))
		if r.is_empty():
			print("OK  ", name)
		else:
			total += 1
			print("BAD ", name, "\n", r)
	print("SCRIPTS WITH MISMATCH: ", total)
	quit(0 if total == 0 else 1)
