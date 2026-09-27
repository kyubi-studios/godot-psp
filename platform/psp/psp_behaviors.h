#pragma once

#include "core/object/object_id.h"
#include "core/templates/local_vector.h"
#include "core/math/vector3.h"

class SceneTree;

// .tscn'de GDScript olmadan davranış: Node3D'ler metadata ile işaretlenir, C++ tarafı her kare günceller.
//   metadata/psp_behavior = "spinner"       metadata/spin_speed = Vector3(derece/sn)
//   metadata/psp_behavior = "orbit_camera"  metadata/orbit_target = Vector3  (Camera3D; analog/d-pad döndürür,
//                                            L/R yakınlaştırır, girdi yoksa kendiliğinden yavaşça döner)
class PSPBehaviors {
	struct Spinner {
		ObjectID node;
		Vector3 speed_rad;
	};
	struct OrbitCamera {
		ObjectID node;
		Vector3 target;
		float yaw = 0.0f;
		float pitch = 0.0f;
		float distance = 1.0f;
	};
	LocalVector<Spinner> spinners;
	LocalVector<OrbitCamera> cameras;
	ObjectID scanned_scene;

	void _scan(class Node *p_node);

public:
	// Ana sahne yüklendiğinde (bir kez) tarar; sonra davranışları ilerletir.
	void update(SceneTree *p_tree, double p_delta);
};
