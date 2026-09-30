extends SceneTree
func _initialize() -> void:
	var d := GDScriptBytecodeDumper.new()
	var listing: String = d.disassemble(load("res://tests/example.gd"))
	var lines := listing.split("\n")
	print("listing: %d lines" % lines.size())
	var h := BytecodeHighlighter.new()
	# 预热
	for l in lines: h.highlight_line(l)
	var t0 := Time.get_ticks_usec()
	var N := 200
	for _i in N:
		for l in lines:
			h.highlight_line(l)
	var dt := Time.get_ticks_usec() - t0
	var per_line := float(dt) / float(N * lines.size())
	print("%d 轮 × %d 行 = %.1f ms  |  每行 %.2f µs" % [N, lines.size(), dt / 1000.0, per_line])
	# 一屏约 50 行
	print("一屏 50 行高亮成本: %.2f ms" % (per_line * 50.0 / 1000.0))
	quit(0)
