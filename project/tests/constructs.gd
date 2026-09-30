extends Node

var arr: Array[String] = ["a", "b"]
var dict := {"k": 1, "j": 2}
var typed_dict: Dictionary[String, int] = {"n": 3}
var vec := Vector2(1.0, 2.0)
var nested := [[1, 2], [3]]

func make() -> Array:
	return [Vector3(1, 2, 3), {"a": [true, false]}]
