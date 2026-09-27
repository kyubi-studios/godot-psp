#include "psp_behaviors.h"

#include "psp_log.h"

#include "core/input/input.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

void PSPBehaviors::_scan(Node *p_node) {
	static const StringName behavior_key = "psp_behavior";
	Node3D *n3d = Object::cast_to<Node3D>(p_node);
	if (n3d && n3d->has_meta(behavior_key)) {
		const String kind = n3d->get_meta(behavior_key);
		if (kind == "spinner") {
			Vector3 deg = n3d->get_meta("spin_speed", Vector3(0, 90, 0));
			spinners.push_back({ n3d->get_instance_id(), Vector3(Math::deg_to_rad(deg.x), Math::deg_to_rad(deg.y), Math::deg_to_rad(deg.z)) });
		} else if (kind == "orbit_camera" && !Object::cast_to<Camera3D>(n3d)) {
			WARN_PRINT(vformat("PSP: psp_behavior \"orbit_camera\" requires a Camera3D (node %s); ignored.", n3d->get_name()));
		} else if (kind == "orbit_camera") {
			OrbitCamera oc;
			oc.node = n3d->get_instance_id();
			oc.target = n3d->get_meta("orbit_target", Vector3());
			const Vector3 offset = n3d->get_position() - oc.target;
			oc.distance = MAX(offset.length(), 0.5f);
			oc.yaw = Math::atan2(offset.x, offset.z);
			oc.pitch = Math::asin(CLAMP(offset.y / oc.distance, -1.0f, 1.0f));
			cameras.push_back(oc);
		}
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_scan(p_node->get_child(i));
	}
}

void PSPBehaviors::update(SceneTree *p_tree, double p_delta) {
	if (!p_tree) {
		return;
	}
	Node *scene = p_tree->get_current_scene();
	if (!scene) {
		return;
	}
	if (scanned_scene != scene->get_instance_id()) {
		spinners.clear();
		cameras.clear();
		_scan(scene);
		scanned_scene = scene->get_instance_id();
		psp_log("[PSP] behaviors spinners=%d cameras=%d", (int)spinners.size(), (int)cameras.size());
	}
	const float dt = (float)p_delta;

	for (const Spinner &s : spinners) {
		Node3D *n = ObjectDB::get_instance<Node3D>(s.node);
		if (n) {
			n->set_rotation(n->get_rotation() + s.speed_rad * dt);
		}
	}

	Input *input = Input::get_singleton();
	float ax = input->get_joy_axis(0, JoyAxis::LEFT_X);
	float ay = input->get_joy_axis(0, JoyAxis::LEFT_Y);
	ax += (input->is_joy_button_pressed(0, JoyButton::DPAD_RIGHT) ? 1.0f : 0.0f) - (input->is_joy_button_pressed(0, JoyButton::DPAD_LEFT) ? 1.0f : 0.0f);
	ay += (input->is_joy_button_pressed(0, JoyButton::DPAD_DOWN) ? 1.0f : 0.0f) - (input->is_joy_button_pressed(0, JoyButton::DPAD_UP) ? 1.0f : 0.0f);
	const float zoom = (input->is_joy_button_pressed(0, JoyButton::RIGHT_SHOULDER) ? -1.0f : 0.0f) + (input->is_joy_button_pressed(0, JoyButton::LEFT_SHOULDER) ? 1.0f : 0.0f);
	const bool idle = ax == 0.0f && ay == 0.0f && zoom == 0.0f;

	for (OrbitCamera &oc : cameras) {
		Camera3D *cam = ObjectDB::get_instance<Camera3D>(oc.node);
		if (!cam) {
			continue;
		}
		oc.yaw += (idle ? 0.25f : ax * 1.8f) * dt;
		oc.pitch = CLAMP(oc.pitch + ay * 1.2f * dt, 0.05f, 1.35f);
		oc.distance = CLAMP(oc.distance + zoom * 4.0f * dt, 2.0f, 20.0f);
		const Vector3 dir(Math::sin(oc.yaw) * Math::cos(oc.pitch), Math::sin(oc.pitch), Math::cos(oc.yaw) * Math::cos(oc.pitch));
		cam->look_at_from_position(oc.target + dir * oc.distance, oc.target, Vector3(0, 1, 0));
	}
}
