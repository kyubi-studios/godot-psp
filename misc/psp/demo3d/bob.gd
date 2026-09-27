extends MeshInstance3D
## Küreyi yukarı aşağı salındırır (PSP'de GDScript örneği).

@export var height := 0.35
@export var speed := 2.0
var _t := 0.0
var _base_y := 0.0

func _ready() -> void:
	_base_y = position.y
	print("[GD] demo bob ready")

func _process(delta: float) -> void:
	_t += delta
	position.y = _base_y + sin(_t * speed) * height
