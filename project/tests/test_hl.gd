extends SceneTree
func _initialize() -> void:
	# 回归用例。含触发过死循环的 const(-1.0)，以及各类会让扫描器停不下来的输入。
	var lines := [
		"Script: res://example.gd",
		"--- _ready ---",
		"    25      2  stack(5) = member(speed) + stack(3)",
		"    26     12  class = <anonymous lambda>()",
		"    19     93  stack(10) = const(-1.0)",
		"    19     93  stack(10) = const(-0.5) + stack(3)",
		"    19     93  member(a) = const(-1)",
		"    19     93  x = \"unterminated",
		"    19     93  x = <unclosed",
		"    19     93  -",
		"    19     93  - ",
		"    19     93  ",
		"    19     93  " + "x".repeat(2000) + " = const(-1.0)",
		"    19     93  <",
		"    19     93  \"",
		"",
	]
	var h := BytecodeHighlighter.new()
	var total := 0
	for i in lines.size():
		var d: Dictionary = h.highlight_line(lines[i])
		total += d.size()
	print("OK: %d lines, %d colour entries" % [lines.size(), total])
	quit(0)
