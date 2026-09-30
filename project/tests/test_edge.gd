extends SceneTree
# The intentionally-broken fixture is written at run time rather than kept in the
# project: the editor parses every .gd under res:// on startup, so a file with a
# syntax error prints a parse error on every launch.
const BROKEN_PATH := "user://disassemble_tests/broken.gd"

func _make_broken_fixture() -> void:
	DirAccess.make_dir_recursive_absolute(BROKEN_PATH.get_base_dir())
	var f := FileAccess.open(BROKEN_PATH, FileAccess.WRITE)
	f.store_string("extends Node\n\nfunc _ready() -> void:\n\tthis is not valid gdscript !!!\n")
	f.close()

func _initialize() -> void:
	_make_broken_fixture()
	var dumper := GDScriptBytecodeDumper.new()
	for p in ["res://tests/empty.gd", BROKEN_PATH, "res://tests/nested.gd", "res://tests/tool_script.gd", "res://tests/example.gd"]:
		print("========== ", p)
		var s: Script = load(p)
		if s == null:
			print("  load returned null")
			continue
		print(dumper.disassemble(s).substr(0, 400))
	# 断言：嵌套类必须被遍历，且成员名要按"所属类"解析（而不是外层脚本）。
	var nested: String = dumper.disassemble(load("res://tests/nested.gd"))
	var failures := []
	if not nested.contains("Inner.f"):
		failures.append("nested class function 'Inner.f' missing")
	if not nested.contains("Inner.@implicit_initializer"):
		failures.append("nested class initializer missing")
	if not nested.contains("return member(x)"):
		failures.append("inner member 'x' not resolved against its owning class")
	# construct 家族：数组/字典/带类型/构造函数都必须解出目标与元素。
	var c: String = dumper.disassemble(load("res://tests/constructs.gd"))
	for probe in ["[const(\"a\"), const(\"b\")]",
			"{const(\"k\"): const(1), const(\"j\"): const(2)}",
			"{const(\"n\"): const(3)}",
			# Vector3(1, 2, 3) is constant-folded by the analyser into a single
			# constant, so there is no construct instruction to find.
			"Vector2()", "const((1.0, 2.0, 3.0))"]:
		if not c.contains(probe):
			failures.append("construct missing: " + probe)

	# 迭代与带类型的返回：这两个操作数位置曾经用错（步长被当成上界）。
	var iter: String = dumper.disassemble(load("res://tests/example.gd"))
	if not iter.contains("in range to "):
		failures.append("ITERATE_RANGE not rendered with 'in range to'")
	# RETURN_TYPED_* only appears when a conversion is genuinely needed at the
	# return, which the fixtures do not reliably trigger -- `return []` for an
	# `Array[Node]` result needs none. So this checks the shape only if such an
	# instruction actually shows up, instead of demanding one exist.
	for name in ["example", "constructs", "objects", "nested"]:
		for line in dumper.disassemble(load("res://tests/%s.gd" % name)).split("\n"):
			if line.contains("return typed") and not line.contains(" as "):
				failures.append("typed return rendered oddly: " + line.strip_edges())

	# 渲染退化探针：槽位算错会导致索引越界，渲染就退化成这些占位符形式。
	# 这是"操作数位置是否与引擎一致"最直接的检验。
	for fb in ["member(#", "type(#", "ctor#", "utility#", "bind#", "lambda#", "<decode failed", "<err>"]:
		for name in ["example", "constructs", "nested", "tool_script", "empty"]:
			var text: String = dumper.disassemble(load("res://tests/%s.gd" % name))
			if text.contains(fb):
				failures.append("unresolved '%s' in %s" % [fb, name])

	# 源码行号列：只要函数里出现了 line 指令，它的每一行就该有行号。曾经因为删
	# 代码块时误删了 OPCODE_LINE 的记录，整个 @implicit_initializer 的行号列
	# 变成 "-"，而当时所有断言都没发现。
	# 注意不能要求所有函数都有行号：空的隐式初始化器只有一条 end，本来就没有。
	for name in ["example", "constructs", "nested", "objects"]:
		var blocks: PackedStringArray = dumper.disassemble(load("res://tests/%s.gd" % name)).split("--- ")
		for b in blocks:
			var body: PackedStringArray = b.split("\n")
			var has_line_instr := false
			for l in body:
				if l.contains(" line "):
					has_line_instr = true
					break
			if not has_line_instr:
				continue
			for l in body:
				if l.length() > 14 and l.substr(0, 6).strip_edges() == "-":
					failures.append("missing source line in %s: %s" % [name, l.strip_edges()])
					break

	# 操作数自检：把渲染器实际读取的槽位/类型与引擎声明的比对。这条抓到过
	# ASSIGN_TYPED_ARRAY / _DICTIONARY 完全没有实现（掉进原始槽位转储）。
	for name in ["example", "constructs", "nested", "objects", "tool_script", "empty"]:
		var r: String = dumper.self_test(load("res://tests/%s.gd" % name))
		if not r.is_empty():
			failures.append("operand self-test on %s:\n%s" % [name, r])

	if failures.is_empty():
		print("NESTED OK")
	else:
		for f in failures:
			print("NESTED FAIL: ", f)
	quit(0 if failures.is_empty() else 1)
