extends Node

const Other := preload("res://tests/nested.gd")
var typed: Array[Other]

func check(n: Node) -> bool:
	return n is Node3D

func make() -> Array[Node]:
	return []
