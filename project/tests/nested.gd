extends Node

class Inner:
	var x := 1
	func f() -> int:
		return x

func make() -> Inner:
	return Inner.new()
