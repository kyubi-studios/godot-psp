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
#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/option_button.h"
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

PackedStringArray StudioScenePreview::picker_paths(const PackedStringArray &p_open, const Array &p_recent) {
	PackedStringArray result;
	for (const String &path : p_open) {
		if (!path.is_empty() && !result.has(path)) {
			result.push_back(path);
		}
	}
	for (const Variant &entry : p_recent) {
		if (entry.get_type() == Variant::STRING) {
			const String path = entry;
			if (!path.is_empty() && !result.has(path)) {
				result.push_back(path);
			}
		}
	}
	return result;
}

void StudioScenePreview::set_scene_path(const String &p_path) {
	scene_path = p_path;
	if (EditorSettings::get_singleton() && EditorNode::get_singleton()) {
		EditorSettings::get_singleton()->set_project_metadata("studio", "preview_scene", scene_path);
	}
}

void StudioScenePreview::_update_picker() {
	PackedStringArray open;
	EditorData &data = EditorNode::get_editor_data();
	for (int i = 0; i < data.get_edited_scene_count(); i++) {
		open.push_back(data.get_scene_path(i));
	}
	picker_items = picker_paths(open, EditorSettings::get_singleton()->get_project_metadata("recent_files", "scenes", Array()));
	if (!scene_path.is_empty() && !picker_items.has(scene_path)) {
		picker_items.insert(0, scene_path);
	}

	picker->clear();
	for (const String &path : picker_items) {
		picker->add_item(path.get_file());
		picker->set_item_tooltip(-1, path);
	}
	picker->add_separator();
	picker->add_item(TTR("Choose File..."));
	picker->select(picker_items.find(scene_path));
}

void StudioScenePreview::_picker_selected(int p_index) {
	if (p_index >= 0 && p_index < picker_items.size()) {
		set_scene_path(picker_items[p_index]);
		refresh();
	} else {
		file_dialog->popup_file_dialog();
	}
}

void StudioScenePreview::_file_chosen(const String &p_path) {
	set_scene_path(p_path);
	refresh();
	_update_picker();
}

void StudioScenePreview::_open_in_editor() {
	if (!scene_path.is_empty()) {
		EditorNode::get_singleton()->load_scene(scene_path);
	}
}

void StudioScenePreview::_scene_saved(const String &p_path) {
	if (auto_refresh->is_pressed() && p_path == scene_path) {
		refresh();
	}
}

void StudioScenePreview::_resources_reimported(const Vector<String> &p_paths) {
	// Reimported dependencies (textures, models) change what the preview shows.
	if (auto_refresh->is_pressed() && preview_root && is_visible_in_tree()) {
		refresh();
	}
}

void StudioScenePreview::connect_editor_signals() {
	EditorNode::get_singleton()->connect("scene_saved", callable_mp(this, &StudioScenePreview::_scene_saved));
	EditorFileSystem::get_singleton()->connect("resources_reimported", callable_mp(this, &StudioScenePreview::_resources_reimported));
	picker->get_popup()->connect("about_to_popup", callable_mp(this, &StudioScenePreview::_update_picker));
	const String saved = EditorSettings::get_singleton()->get_project_metadata("studio", "preview_scene", String());
	if (!saved.is_empty()) {
		scene_path = saved;
		_update_picker();
		callable_mp(this, &StudioScenePreview::refresh).call_deferred();
	}
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
		const bool has_light = _has_node_of_type(preview_root, SNAME("Light3D"));
		const bool has_env = _has_node_of_type(preview_root, SNAME("WorldEnvironment"));
		default_lighting->get_child(0)->set("visible", !has_light);
		default_lighting->get_child(1)->set("environment", has_env ? Variant() : default_lighting->get_child(1)->get_meta("default_environment"));
		default_lighting->set_visible(true);
		camera->set_current(true);
	} else {
		default_lighting->set_visible(false);
	}
	zoom = 1.0;
	pan = Vector2();
	distance_scale = 1.2;
	_fit_view();
	// Some nodes (e.g. CSG shapes) build their meshes deferred after entering the tree; fit again then.
	callable_mp(this, &StudioScenePreview::_fit_view).call_deferred();
}

void StudioScenePreview::_fit_view() {
	if (!preview_root) {
		return;
	}
	bool found = false;
	if (preview_is_3d) {
		bounds = AABB();
		_collect_bounds_3d(preview_root, Transform3D(), bounds, found);
		if (!found || bounds.size.length() < 0.01) {
			bounds = AABB(bounds.get_center() - Vector3(2, 2, 2), Vector3(4, 4, 4)); // Nothing visible yet.
		}
		_update_camera();
	} else {
		content_rect = Rect2();
		_collect_bounds_2d(preview_root, Transform2D(), content_rect, found);
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

	HBoxContainer *toolbar = memnew(HBoxContainer);
	main_vb->add_child(toolbar);

	picker = memnew(OptionButton);
	picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	picker->set_clip_text(true);
	picker->set_fit_to_longest_item(false);
	picker->set_tooltip_text(TTRC("Scene to preview"));
	picker->set_accessibility_name(TTRC("Scene to preview"));
	picker->connect(SceneStringName(item_selected), callable_mp(this, &StudioScenePreview::_picker_selected));
	toolbar->add_child(picker);

	Button *refresh_button = memnew(Button);
	refresh_button->set_flat(true);
	refresh_button->set_text(TTRC("Reload"));
	refresh_button->set_tooltip_text(TTRC("Reload the previewed scene from disk."));
	refresh_button->connect(SceneStringName(pressed), callable_mp(this, &StudioScenePreview::refresh));
	toolbar->add_child(refresh_button);

	open_button = memnew(Button);
	open_button->set_flat(true);
	open_button->set_text(TTRC("Edit"));
	open_button->set_tooltip_text(TTRC("Open the previewed scene in the editor."));
	open_button->connect(SceneStringName(pressed), callable_mp(this, &StudioScenePreview::_open_in_editor));
	toolbar->add_child(open_button);

	auto_refresh = memnew(CheckBox);
	auto_refresh->set_text(TTRC("Auto"));
	auto_refresh->set_pressed(true);
	auto_refresh->set_tooltip_text(TTRC("Reload automatically when the scene is saved or its resources are reimported."));
	toolbar->add_child(auto_refresh);

	file_dialog = memnew(EditorFileDialog);
	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->add_filter("*.tscn,*.scn", TTR("Scenes"));
	file_dialog->connect("file_selected", callable_mp(this, &StudioScenePreview::_file_chosen));
	add_child(file_dialog);

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
