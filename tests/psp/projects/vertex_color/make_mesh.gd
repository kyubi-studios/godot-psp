# Godot 4.7 editörüyle bir kez çalıştırılır: vertex rengi (yeşil) olan bir üçgen ArrayMesh üretir.
# godot --headless --path tests/psp/projects/vertex_color --script res://make_mesh.gd
extends SceneTree

func _init() -> void:
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([Vector3(-1, -1, 0), Vector3(0, 1, 0), Vector3(1, -1, 0)])  # Godot ön yüzü: saat yönü
	arrays[Mesh.ARRAY_NORMAL] = PackedVector3Array([Vector3(0, 0, 1), Vector3(0, 0, 1), Vector3(0, 0, 1)])
	arrays[Mesh.ARRAY_COLOR] = PackedColorArray([Color(0, 1, 0), Color(0, 1, 0), Color(0, 1, 0)])
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	ResourceSaver.save(mesh, "res://colored_tri.tres")
	quit()
