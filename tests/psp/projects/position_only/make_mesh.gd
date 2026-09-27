# godot --headless --path tests/psp/projects/position_only --script res://make_mesh.gd
# Yalnızca pozisyon içeren (normal/UV/renk yok) üçgen ArrayMesh.
extends SceneTree

func _init() -> void:
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([Vector3(-1, -1, 0), Vector3(0, 1, 0), Vector3(1, -1, 0)])
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	ResourceSaver.save(mesh, "res://pos_tri.tres")
	quit()
