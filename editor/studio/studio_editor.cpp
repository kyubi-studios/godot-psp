/**************************************************************************/
/*  studio_editor.cpp                                                     */
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


#include "studio_editor.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/settings/editor_settings.h"
#include "editor/studio/studio_drawer.h"
#include "editor/studio/studio_pages_bar.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/main/viewport.h"

void StudioEditor::register_settings() {
	EDITOR_DEF("interface/studio/pages/auto_save_on_switch", true);
	EDITOR_DEF("interface/studio/bottom_drawer/auto_hide", false);

	for (int i = 1; i <= PAGE_SHORTCUT_COUNT; i++) {
		ED_SHORTCUT(vformat("studio/page_%d", i), vformat(TTR("Switch to Layout Page %d"), i), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key(int(Key::KEY_1) + i - 1));
	}

	ED_SHORTCUT("studio/toggle_left_docks", TTRC("Toggle Left Docks"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::COMMA);
	ED_SHORTCUT("studio/toggle_right_docks", TTRC("Toggle Right Docks"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::PERIOD);
}

void StudioEditor::setup(EditorTitleBar *p_title_bar, Control *p_title_right_container, EditorBottomPanel *p_bottom_panel) {
	title_bar = p_title_bar;
	title_right_container = p_title_right_container;
	bottom_panel = p_bottom_panel;

	register_settings();

	pages_bar = memnew(StudioPagesBar);
	pages_bar->connect("layouts_changed", callable_mp(this, &StudioEditor::_layouts_changed));
	title_right_container->add_child(pages_bar);
	title_right_container->move_child(pages_bar, 0);
	pages_bar->set_current_page(EditorSettings::get_singleton()->get_project_metadata("studio", "current_page", String()));

	left_docks_button = memnew(Button);
	left_docks_button->set_toggle_mode(true);
	left_docks_button->set_pressed_no_signal(true);
	left_docks_button->set_theme_type_variation("FlatButton");
	left_docks_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	left_docks_button->set_shortcut(ED_GET_SHORTCUT("studio/toggle_left_docks"));
	left_docks_button->set_shortcut_in_tooltip(true);
	left_docks_button->set_tooltip_text(TTRC("Show or hide the left docks."));
	left_docks_button->set_accessibility_name(TTRC("Toggle Left Docks"));
	left_docks_button->connect(SceneStringName(pressed), callable_mp(this, &StudioEditor::_toggle_dock_region).bind(EditorDockManager::DOCK_REGION_LEFT));
	title_right_container->add_child(left_docks_button);
	title_right_container->move_child(left_docks_button, pages_bar->get_index() + 1);

	right_docks_button = memnew(Button);
	right_docks_button->set_toggle_mode(true);
	right_docks_button->set_pressed_no_signal(true);
	right_docks_button->set_theme_type_variation("FlatButton");
	right_docks_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	right_docks_button->set_shortcut(ED_GET_SHORTCUT("studio/toggle_right_docks"));
	right_docks_button->set_shortcut_in_tooltip(true);
	right_docks_button->set_tooltip_text(TTRC("Show or hide the right docks."));
	right_docks_button->set_accessibility_name(TTRC("Toggle Right Docks"));
	right_docks_button->connect(SceneStringName(pressed), callable_mp(this, &StudioEditor::_toggle_dock_region).bind(EditorDockManager::DOCK_REGION_RIGHT));
	title_right_container->add_child(right_docks_button);
	title_right_container->move_child(right_docks_button, left_docks_button->get_index() + 1);
	right_docks_button->connect(SceneStringName(theme_changed), callable_mp(this, &StudioEditor::_update_icons));

	// Regions can also be shown by focusing one of their docks, so follow the manager's state.
	EditorDockManager::get_singleton()->connect("dock_region_visibility_changed", callable_mp(this, &StudioEditor::_update_dock_region_buttons).unbind(2));

	drawer = memnew(StudioDrawer);
	drawer->setup(bottom_panel);
	add_child(drawer);

	set_process_shortcut_input(true);
}

void StudioEditor::notify_layouts_changed() {
	if (pages_bar) {
		pages_bar->refresh();
	}
}

void StudioEditor::_toggle_dock_region(int p_region) {
	EditorDockManager *dock_manager = EditorDockManager::get_singleton();
	ERR_FAIL_NULL(dock_manager);
	const EditorDockManager::DockRegion region = EditorDockManager::DockRegion(p_region);
	dock_manager->set_dock_region_visible(region, !dock_manager->is_dock_region_visible(region));
}

void StudioEditor::_update_dock_region_buttons() {
	EditorDockManager *dock_manager = EditorDockManager::get_singleton();
	if (!dock_manager || !left_docks_button) {
		return;
	}
	left_docks_button->set_pressed_no_signal(dock_manager->is_dock_region_visible(EditorDockManager::DOCK_REGION_LEFT));
	right_docks_button->set_pressed_no_signal(dock_manager->is_dock_region_visible(EditorDockManager::DOCK_REGION_RIGHT));
}

void StudioEditor::_update_icons() {
	left_docks_button->set_button_icon(left_docks_button->get_editor_theme_icon(SNAME("Panels2")));
	right_docks_button->set_button_icon(right_docks_button->get_editor_theme_icon(SNAME("Panels2Alt")));
}

void StudioEditor::_layouts_changed() {
	emit_signal(SNAME("layouts_changed"));
}

void StudioEditor::shortcut_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed() || k->is_echo() || !pages_bar) {
		return;
	}
	for (int i = 1; i <= PAGE_SHORTCUT_COUNT; i++) {
		if (ED_IS_SHORTCUT(vformat("studio/page_%d", i), p_event)) {
			pages_bar->switch_to_index(i - 1);
			get_viewport()->set_input_as_handled();
			return;
		}
	}
}

void StudioEditor::_bind_methods() {
	ADD_SIGNAL(MethodInfo("layouts_changed"));
}

StudioEditor::StudioEditor() {
	singleton = this;
	set_name("StudioEditor");
}

StudioEditor::~StudioEditor() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
