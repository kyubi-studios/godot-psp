/**************************************************************************/
/*  studio_editor.h                                                       */
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

#include "editor/studio/studio_scene_cache.h"
#include "scene/main/node.h"

class Button;
class Control;
class EditorBottomPanel;
class EditorTitleBar;
class StudioDrawer;
class StudioPagesBar;

// Root of the Studio editor extensions. Owned by EditorNode; everything Studio adds
// to the editor UI is created and wired from here so upstream files only need one hook.
class StudioEditor : public Node {
	GDCLASS(StudioEditor, Node);

	static constexpr int PAGE_SHORTCUT_COUNT = 9;

	static inline StudioEditor *singleton = nullptr;

	EditorTitleBar *title_bar = nullptr;
	Control *title_right_container = nullptr;
	EditorBottomPanel *bottom_panel = nullptr;
	StudioPagesBar *pages_bar = nullptr;
	Button *left_docks_button = nullptr;
	Button *right_docks_button = nullptr;
	StudioDrawer *drawer = nullptr;
	StudioSceneCache scene_cache;

	void _layouts_changed();
	void _editor_settings_changed();
	void _toggle_dock_region(int p_region);
	void _update_dock_region_buttons();
	void _update_icons();

protected:
	static void _bind_methods();
	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

public:
	static StudioEditor *get_singleton() { return singleton; }

	// Registers Studio editor settings and shortcuts. Requires EditorSettings.
	static void register_settings();

	void setup(EditorTitleBar *p_title_bar, Control *p_title_right_container, EditorBottomPanel *p_bottom_panel);

	// Called by EditorNode when the stock "Editor Layout" menu changed the layouts file.
	void notify_layouts_changed();
	// Called by EditorNode after the stock "Editor Layout" menu loaded a layout.
	void notify_layout_loaded(const String &p_layout);

	// Called by EditorNode right before a scene tab is closed, while the scene is still loaded.
	void notify_scene_closing(const String &p_path);
	// Called by EditorNode after a scene was opened successfully.
	void notify_scene_opened(const String &p_path);
	const StudioSceneCache &get_scene_cache() const { return scene_cache; }

	StudioPagesBar *get_pages_bar() const { return pages_bar; }

	StudioEditor();
	~StudioEditor();
};
