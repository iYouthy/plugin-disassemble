extends SceneTree
func _initialize() -> void:
	for n in ["example", "objects", "constructs"]:
		var t: String = GDScriptBytecodeDumper.new().disassemble(load("res://tests/%s.gd" % n))
		print("########## ", n)
		var lines := t.split("\n")
		var started := false
		for l in lines:
			if l.begins_with("--- "):
				started = l.contains("implicit_initializer")
				if started: print(l)
				elif started: pass
			elif started and l.strip_edges() != "":
				print(l)
				if l.contains("end") and l.strip_edges().ends_with("end"): break
	quit(0)
