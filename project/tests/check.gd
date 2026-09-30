extends SceneTree
func _initialize() -> void:
	var d := GDScriptBytecodeDumper.new()
	# 解析失败的占位符：槽位算错时索引越界，渲染就会退化成这些形式。
	var fallbacks := ["member(#", "type(#", "ctor#", "utility#", "bind#", "lambda#", "<decode failed", "<err>"]
	var files := ["example", "constructs", "nested", "tool_script", "empty"]
	var bad := []
	for f in files:
		var text: String = d.disassemble(load("res://tests/%s.gd" % f))
		for fb in fallbacks:
			if text.contains(fb):
				var line := ""
				for l in text.split("\n"):
					if l.contains(fb):
						line = l.strip_edges()
						break
				bad.append("%s: '%s' -> %s" % [f, fb, line])
	for b in bad:
		print("FALLBACK ", b)
	print("FALLBACKS ", bad.size())
	quit(0 if bad.is_empty() else 1)
