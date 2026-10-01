/**************************************************************************/
/*  studio_scene_preview.h                                                */
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


#pragma once

#include "editor/docks/editor_dock.h"

class Button;
class Camera3D;
class CheckBox;
class EditorFileDialog;
class OptionButton;
class InputEvent;
class Label;
class Node3D;
class SubViewport;
class SubViewportContainer;
class VBoxContainer;

// "Scene Preview" dock: a live, read-only view of another scene next to the one being edited.
// The scene is instantiated from disk into its own world, so the edited scene is never touched.
class StudioScenePreview : public EditorDock {
	GDCLASS(StudioScenePreview, EditorDock);

	String scene_path;
	String error;
	Node *preview_root = nullptr;
	bool preview_is_3d = false;

	VBoxContainer *main_vb = nullptr;
	SubViewportContainer *container = nullptr;
	SubViewport *viewport = nullptr;
	Camera3D *camera = nullptr;
	Node3D *default_lighting = nullptr;
	Label *message = nullptr;

	OptionButton *picker = nullptr;
	PackedStringArray picker_items;
	Button *open_button = nullptr;
	CheckBox *auto_refresh = nullptr;
	EditorFileDialog *file_dialog = nullptr;

	// 3D navigation.
	AABB bounds;
	float yaw = 0.6;
	float pitch = -0.45;
	float distance_scale = 1.2;
	// 2D navigation.
	Rect2 content_rect;
	float zoom = 1.0;
	Vector2 pan;

	void _clear();
	void _show_message(const String &p_text);
	void _fit_view();
	void _update_camera();
	void _update_canvas();
	void _update_render_mode();
	void _viewport_input(const Ref<InputEvent> &p_event);

	void _update_picker();
	void _picker_selected(int p_index);
	void _file_chosen(const String &p_path);
	void _open_in_editor();
	void _scene_saved(const String &p_path);
	void _resources_reimported(const Vector<String> &p_paths);

protected:
	void _notification(int p_what);

public:
	// Camera transform orbiting the center of p_bounds (origin when empty), looking at it.
	static Transform3D orbit_transform(const AABB &p_bounds, float p_yaw, float p_pitch, float p_distance_scale);
	// Canvas transform that fits p_content into 90% of p_viewport, then applies zoom and pan.
	static Transform2D fit_canvas_transform(const Rect2 &p_content, const Size2 &p_viewport, float p_zoom, const Vector2 &p_pan);

	// Open scene tabs first, then recent scenes; unique, non-empty strings only.
	static PackedStringArray picker_paths(const PackedStringArray &p_open, const Array &p_recent);

	// Connects to editor signals (scene saved, reimport). Called by StudioEditor once the editor exists.
	void connect_editor_signals();

	void set_scene_path(const String &p_path);
	String get_scene_path() const { return scene_path; }
	void refresh();

	Node *get_preview_root() const { return preview_root; }
	bool is_3d() const { return preview_is_3d; }
	String get_error() const { return error; }

	VBoxContainer *get_main_container() const { return main_vb; }
	SubViewport *get_preview_viewport() const { return viewport; }

	// True when p_scene instances or otherwise depends on p_dependency, directly or indirectly.
	static bool depends_on(const String &p_scene, const String &p_dependency);

	StudioScenePreview();
};
