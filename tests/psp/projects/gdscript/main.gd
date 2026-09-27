extends Node2D

var frames := 0
@onready var box: ColorRect = $Box

func _ready() -> void:
	var values := [1, 2, 3].map(func(x): return x * 14)
	print("[GD] ready sum=", values.reduce(func(a, b): return a + b))

func _process(_delta: float) -> void:
	frames += 1
	box.position.x = 20 + frames * 4
	if frames == 30:
		print("[GD] frame30 box_x=", box.position.x)
