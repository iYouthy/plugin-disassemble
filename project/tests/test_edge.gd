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

	if failures.is_empty():
		print("NESTED OK")
	else:
		for f in failures:
			print("NESTED FAIL: ", f)
	# 渲染退化探针：槽位算错会导致索引越界，渲染就退化成这些占位符形式。
	# 这是"操作数位置是否与引擎一致"最直接的检验。
	for fb in ["member(#", "type(#", "ctor#", "utility#", "bind#", "lambda#", "<decode failed", "<err>"]:
		for name in ["example", "constructs", "nested", "tool_script", "empty"]:
			var text: String = dumper.disassemble(load("res://tests/%s.gd" % name))
			if text.contains(fb):
				failures.append("unresolved '%s' in %s" % [fb, name])

	quit(0 if failures.is_empty() else 1)
