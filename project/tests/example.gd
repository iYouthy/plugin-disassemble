extends Node

const MAX_SPEED := 120.0
const LABEL := "player"

var speed: float = 10.0
var names: Array[String] = ["a", "b"]
var lookup := {"x": 1, "y": 2}

signal moved(distance: float)


func _ready() -> void:
	var total := 0.0
	for n in names:
		total += n.length()
	for i in range(3):
		total += i
	speed = MAX_SPEED if total > 10.0 else -1.0
	moved.emit(speed)
	print(LABEL, lookup.get("x", 0), total)


func _process(delta: float) -> void:
	speed += delta
	var double := func(x: int) -> int:
		return x * 2
	speed = double.call(21)
