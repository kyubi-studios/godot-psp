/**************************************************************************/
/*  studio_scene_preview.cpp                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/


#include "studio_scene_preview.h"

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "scene/2d/node_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/gui/box_container.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/viewport.h"
#include "scene/resources/environment.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/sky.h"
#include "scene/resources/3d/sky_material.h"

Transform3D StudioScenePreview::orbit_transform(const AABB &p_bounds, float p_yaw, float p_pitch, float p_distance_scale) {
	const Vector3 target = p_bounds.has_volume() || p_bounds.size != Vector3() ? p_bounds.get_center() : Vector3();
	const real_t distance = MAX(0.5, p_bounds.size.length() * p_distance_scale);
	const Vector3 offset = Vector3(Math::sin(p_yaw) * Math::cos(p_pitch), -Math::sin(p_pitch), Math::cos(p_yaw) * Math::cos(p_pitch)) * distance;
	Transform3D xf;
	xf.origin = target + offset;
	xf.basis = Basis::looking_at(target - xf.origin, Vector3(0, 1, 0));
	return xf;
}

Transform2D StudioScenePreview::fit_canvas_transform(const Rect2 &p_content, const Size2 &p_viewport, float p_zoom, const Vector2 &p_pan) {
	real_t scale = 1.0;
	if (p_content.has_area() && p_viewport.x > 0 && p_viewport.y > 0) {
		scale = MIN(p_viewport.x * 0.9 / p_content.size.x, p_viewport.y * 0.9 / p_content.size.y);
	}
	scale *= p_zoom;
	const Vector2 origin = p_viewport / 2 - p_content.get_center() * scale + p_pan;
	return Transform2D(0, Size2(scale, scale), 0, origin);
}

// Bounds of all visual instances, accumulating local transforms (works outside the scene tree).
static void _collect_bounds_3d(Node *p_node, const Transform3D &p_parent, AABB &r_bounds, bool &r_found) {
	Transform3D xf = p_parent;
	if (Node3D *n3d = Object::cast_to<Node3D>(p_node)) {
		xf = p_parent * n3d->get_transform();
	}
	if (VisualInstance3D *vi = Object::cast_to<VisualInstance3D>(p_node)) {
		const AABB aabb = xf.xform(vi->get_aabb());
		if (r_found) {
			r_bounds.merge_with(aabb);
		} else {
			r_bounds = aabb;
			r_found = true;
		}
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_collect_bounds_3d(p_node->get_child(i), xf, r_bounds, r_found);
	}
}

static void _collect_bounds_2d(Node *p_node, const Transform2D &p_parent, Rect2 &r_rect, bool &r_found) {
	Transform2D xf = p_parent;
	Rect2 local;
	bool has_local = false;
	if (Node2D *n2d = Object::cast_to<Node2D>(p_node)) {
		xf = p_parent * n2d->get_transform();
		if (Sprite2D *sprite = Object::cast_to<Sprite2D>(p_node)) {
			local = sprite->get_rect();
		}
		has_local = true;
	} else if (Control *control = Object::cast_to<Control>(p_node)) {
		xf = p_parent * control->get_transform();
		local = Rect2(Point2(), control->get_size());
		has_local = true;
	}
	if (has_local) {
		const Rect2 rect = xf.xform(local);
		if (r_found) {
			r_rect = r_rect.merge(rect);
		} else {
			r_rect = rect;
			r_found = true;
		}
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_collect_bounds_2d(p_node->get_child(i), xf, r_rect, r_found);
	}
}

static bool _has_node_of_type(Node *p_node, const StringName &p_class) {
	if (p_node->is_class(p_class)) {
		return true;
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		if (_has_node_of_type(p_node->get_child(i), p_class)) {
			return true;
		}
	}
	return false;
}

void StudioScenePreview::_clear() {
	if (preview_root) {
		viewport->remove_child(preview_root);
		memdelete(preview_root);
		preview_root = nullptr;
	}
	preview_is_3d = false;
	error = String();
}

void StudioScenePreview::_show_message(const String &p_text) {
	message->set_text(p_text);
	message->set_visible(!p_text.is_empty());
}

void StudioScenePreview::set_scene_path(const String &p_path) {
	scene_path = p_path;
}

void StudioScenePreview::refresh() {
	_clear();
	if (scene_path.is_empty()) {
		_show_message(TTR("Choose a scene to preview."));
		return;
	}
	if (!FileAccess::exists(scene_path)) {
		error = vformat(TTR("Scene file not found: %s"), scene_path);
		_show_message(error);
		return;
	}
	// Read the scene file fresh from disk; dependencies are reused from the resource cache.
	Ref<PackedScene> scene = ResourceLoader::load(scene_path, "PackedScene", ResourceFormatLoader::CACHE_MODE_IGNORE);
	if (scene.is_null() || !scene->can_instantiate()) {
		error = vformat(TTR("Could not load scene: %s"), scene_path);
		_show_message(error);
		return;
	}
	preview_root = scene->instantiate(PackedScene::GEN_EDIT_STATE_DISABLED);
	if (!preview_root) {
		error = vformat(TTR("Could not instantiate scene: %s"), scene_path);
		_show_message(error);
		return;
	}
	viewport->add_child(preview_root);
	_show_message(String());

	preview_is_3d = Object::cast_to<Node3D>(preview_root) != nullptr;
	if (preview_is_3d) {
		bool found = false;
		bounds = AABB();
		_collect_bounds_3d(preview_root, Transform3D(), bounds, found);
		const bool has_light = _has_node_of_type(preview_root, SNAME("Light3D"));
		const bool has_env = _has_node_of_type(preview_root, SNAME("WorldEnvironment"));
		default_lighting->get_child(0)->set("visible", !has_light);
		default_lighting->get_child(1)->set("environment", has_env ? Variant() : default_lighting->get_child(1)->get_meta("default_environment"));
		default_lighting->set_visible(true);
		camera->set_current(true);
		_update_camera();
	} else {
		default_lighting->set_visible(false);
		bool found = false;
		content_rect = Rect2();
		_collect_bounds_2d(preview_root, Transform2D(), content_rect, found);
		zoom = 1.0;
		pan = Vector2();
		_update_canvas();
	}
}

void StudioScenePreview::_update_camera() {
	camera->set_transform(orbit_transform(bounds, yaw, pitch, distance_scale));
	const real_t radius = MAX(1.0, bounds.size.length());
	camera->set_near(MAX(0.01, radius * 0.001));
	camera->set_far(radius * 100.0 + 100.0);
}

void StudioScenePreview::_update_canvas() {
	if (!viewport->is_inside_tree()) {
		return; // Applied again on NOTIFICATION_ENTER_TREE.
	}
	viewport->set_canvas_transform(fit_canvas_transform(content_rect, viewport->get_size(), zoom, pan));
}

void StudioScenePreview::_update_render_mode() {
	// No rendering cost while the dock is hidden or closed.
	viewport->set_update_mode(is_visible_in_tree() ? SubViewport::UPDATE_WHEN_VISIBLE : SubViewport::UPDATE_DISABLED);
}

void StudioScenePreview::_viewport_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && (mm->get_button_mask().has_flag(MouseButtonMask::RIGHT) || mm->get_button_mask().has_flag(MouseButtonMask::MIDDLE))) {
		if (preview_is_3d) {
			yaw -= mm->get_relative().x * 0.01;
			pitch = CLAMP(pitch - mm->get_relative().y * 0.01, -1.5, 1.5);
			_update_camera();
		} else {
			pan += mm->get_relative();
			_update_canvas();
		}
		accept_event();
		return;
	}
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && (mb->get_button_index() == MouseButton::WHEEL_UP || mb->get_button_index() == MouseButton::WHEEL_DOWN)) {
		const float factor = mb->get_button_index() == MouseButton::WHEEL_UP ? 0.9 : 1.1;
		if (preview_is_3d) {
			distance_scale = CLAMP(distance_scale * factor, 0.05, 20.0);
			_update_camera();
		} else {
			zoom = CLAMP(zoom / factor, 0.05, 50.0);
			_update_canvas();
		}
		accept_event();
	}
}

void StudioScenePreview::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_update_render_mode();
			// Children (the viewport) enter the tree after this notification.
			callable_mp(this, &StudioScenePreview::_update_canvas).call_deferred();
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			_update_render_mode();
		} break;
	}
}

StudioScenePreview::StudioScenePreview() {
	set_name("StudioScenePreview");
	set_title(TTRC("Scene Preview"));
	set_layout_key("StudioScenePreview");
	set_icon_name("PackedScene");
	set_default_slot(EditorDock::DOCK_SLOT_RIGHT_BL);
	set_available_layouts(EditorDock::DOCK_LAYOUT_ALL);

	main_vb = memnew(VBoxContainer);
	add_child(main_vb);

	container = memnew(SubViewportContainer);
	container->set_stretch(true);
	container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	container->set_custom_minimum_size(Size2(64, 64));
	container->connect(SceneStringName(gui_input), callable_mp(this, &StudioScenePreview::_viewport_input));
	container->connect(SceneStringName(resized), callable_mp(this, &StudioScenePreview::_update_canvas));
	main_vb->add_child(container);

	viewport = memnew(SubViewport);
	viewport->set_use_own_world_3d(true);
	viewport->set_handle_input_locally(false);
	viewport->set_update_mode(SubViewport::UPDATE_DISABLED);
	container->add_child(viewport);

	camera = memnew(Camera3D);
	viewport->add_child(camera);

	// Used when the previewed scene brings no light or environment of its own.
	default_lighting = memnew(Node3D);
	viewport->add_child(default_lighting);
	DirectionalLight3D *sun = memnew(DirectionalLight3D);
	sun->set_transform(Transform3D(Basis::from_euler(Vector3(-0.9, 0.6, 0)), Vector3()));
	sun->set_shadow(true);
	default_lighting->add_child(sun);
	WorldEnvironment *world_env = memnew(WorldEnvironment);
	Ref<Environment> env;
	env.instantiate();
	Ref<Sky> sky;
	sky.instantiate();
	Ref<ProceduralSkyMaterial> sky_material;
	sky_material.instantiate();
	sky->set_material(sky_material);
	env->set_background(Environment::BG_SKY);
	env->set_sky(sky);
	env->set_tonemapper(Environment::TONE_MAPPER_FILMIC);
	world_env->set_environment(env);
	world_env->set_meta("default_environment", env);
	default_lighting->add_child(world_env);

	message = memnew(Label);
	message->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	message->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	message->set_text(TTR("Choose a scene to preview."));
	main_vb->add_child(message);
}
