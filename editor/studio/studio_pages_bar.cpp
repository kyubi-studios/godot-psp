/**************************************************************************/
/*  studio_pages_bar.cpp                                                  */
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


#include "studio_pages_bar.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "editor/studio/studio_layout_pages.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"

Ref<ConfigFile> StudioPagesBar::_load_config() const {
	Ref<ConfigFile> config;
	config.instantiate();
	// A missing or unreadable file simply means there are no pages yet.
	if (config->load(config_path) != OK) {
		config.instantiate();
	}
	return config;
}

Error StudioPagesBar::_save_config(const Ref<ConfigFile> &p_config) {
	Error err = p_config->save(config_path);
	if (err != OK) {
		ERR_PRINT(vformat("Studio: Could not save layout pages to \"%s\".", config_path));
		return err;
	}
	_layouts_changed();
	return OK;
}

void StudioPagesBar::_layouts_changed() {
	refresh();
	emit_signal(SNAME("layouts_changed"));
}

void StudioPagesBar::set_config_path(const String &p_path) {
	config_path = p_path;
}

String StudioPagesBar::get_config_path() const {
	return config_path;
}

void StudioPagesBar::refresh() {
	pages = StudioLayoutPages::list_pages(_load_config());

	for (Button *button : page_buttons) {
		remove_child(button);
		button->queue_free();
	}
	page_buttons.clear();

	for (const String &page : pages) {
		Button *button = memnew(Button);
		button->set_text(page);
		button->set_toggle_mode(true);
		button->set_button_group(button_group);
		button->set_theme_type_variation("FlatButton");
		button->set_focus_mode(FOCUS_ACCESSIBILITY);
		button->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		button->set_pressed_no_signal(page == current_page);
		button->set_tooltip_text(TTR("Layout page. Right-click for options."));
		button->connect(SceneStringName(pressed), callable_mp(this, &StudioPagesBar::_page_button_pressed).bind(page));
		button->connect(SceneStringName(gui_input), callable_mp(this, &StudioPagesBar::_page_button_gui_input).bind(page));
		add_child(button);
		move_child(button, add_button->get_index());
		page_buttons.push_back(button);
	}
}

void StudioPagesBar::set_current_page(const String &p_page) {
	current_page = p_page;
	if (EditorSettings::get_singleton()) {
		EditorSettings::get_singleton()->set_project_metadata("studio", "current_page", current_page);
	}
	refresh();
}

String StudioPagesBar::get_current_page() const {
	return current_page;
}

void StudioPagesBar::switch_to_page(const String &p_page) {
	EditorDockManager *dock_manager = EditorDockManager::get_singleton();
	ERR_FAIL_NULL(dock_manager);

	Ref<ConfigFile> config = _load_config();
	if (!config->has_section(p_page)) {
		refresh();
		return;
	}

	const bool auto_save = EDITOR_GET("interface/studio/pages/auto_save_on_switch");
	if (auto_save && !current_page.is_empty() && current_page != p_page && config->has_section(current_page)) {
		dock_manager->save_docks_to_config(config, current_page);
		_save_config(config);
	}

	dock_manager->load_docks_from_config(config, p_page);
	if (EditorNode::get_singleton()) {
		EditorNode::get_singleton()->save_editor_layout_delayed();
	}
	set_current_page(p_page);
	emit_signal(SNAME("page_changed"), p_page);
}

void StudioPagesBar::switch_to_index(int p_index) {
	if (p_index < 0 || p_index >= pages.size()) {
		return;
	}
	switch_to_page(pages[p_index]);
}

Error StudioPagesBar::save_current_layout_as(const String &p_page) {
	EditorDockManager *dock_manager = EditorDockManager::get_singleton();
	ERR_FAIL_NULL_V(dock_manager, ERR_UNCONFIGURED);
	const String page = p_page.strip_edges();
	ERR_FAIL_COND_V(!StudioLayoutPages::validate_page_name(page).is_empty(), ERR_INVALID_PARAMETER);

	Ref<ConfigFile> config = _load_config();
	// Drop stale subsections of an overwritten page before saving into it.
	StudioLayoutPages::erase_page(config, page);
	dock_manager->save_docks_to_config(config, page);
	Error err = _save_config(config);
	if (err == OK) {
		set_current_page(page);
	}
	return err;
}

Error StudioPagesBar::rename_page(const String &p_from, const String &p_to) {
	const String to = p_to.strip_edges();
	Ref<ConfigFile> config = _load_config();
	Error err = StudioLayoutPages::rename_page(config, p_from, to);
	if (err != OK) {
		return err;
	}
	err = _save_config(config);
	if (err == OK && current_page == p_from) {
		set_current_page(to);
	}
	return err;
}

void StudioPagesBar::delete_page(const String &p_page) {
	Ref<ConfigFile> config = _load_config();
	StudioLayoutPages::erase_page(config, p_page);
	if (_save_config(config) == OK && current_page == p_page) {
		set_current_page(String());
	}
}

void StudioPagesBar::_page_button_pressed(const String &p_page) {
	switch_to_page(p_page);
}

void StudioPagesBar::_page_button_gui_input(const Ref<InputEvent> &p_event, const String &p_page) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_null() || !mb->is_pressed() || mb->get_button_index() != MouseButton::RIGHT) {
		return;
	}
	context_page = p_page;
	context_menu->set_position(get_screen_position() + get_local_mouse_position());
	context_menu->reset_size();
	context_menu->popup();
	accept_event();
}

void StudioPagesBar::_context_option(int p_option) {
	switch (p_option) {
		case CONTEXT_SAVE_HERE: {
			save_current_layout_as(context_page);
		} break;
		case CONTEXT_RENAME: {
			_popup_name_dialog(NAME_DIALOG_RENAME, context_page);
		} break;
		case CONTEXT_DELETE: {
			delete_page(context_page);
		} break;
	}
}

void StudioPagesBar::_add_button_pressed() {
	_popup_name_dialog(NAME_DIALOG_NEW, String());
}

void StudioPagesBar::_popup_name_dialog(NameDialogMode p_mode, const String &p_initial) {
	name_dialog_mode = p_mode;
	name_dialog->set_title(p_mode == NAME_DIALOG_NEW ? TTR("New Layout Page") : TTR("Rename Layout Page"));
	name_edit->set_text(p_initial);
	_name_text_changed(p_initial);
	name_dialog->popup_centered(Size2(320, 0) * EDSCALE);
	name_edit->select_all();
	name_edit->grab_focus();
}

void StudioPagesBar::_name_text_changed(const String &p_text) {
	String error = StudioLayoutPages::validate_page_name(p_text);
	const String name = p_text.strip_edges();
	if (error.is_empty() && pages.has(name) && !(name_dialog_mode == NAME_DIALOG_RENAME && name == context_page)) {
		error = TTR("A page with this name already exists.");
	}
	name_error->set_text(error);
	name_error->set_visible(!error.is_empty());
	name_dialog->get_ok_button()->set_disabled(!error.is_empty());
}

void StudioPagesBar::_name_confirmed() {
	const String name = name_edit->get_text().strip_edges();
	if (name_dialog_mode == NAME_DIALOG_NEW) {
		save_current_layout_as(name);
	} else {
		rename_page(context_page, name);
	}
}

void StudioPagesBar::_bind_methods() {
	ADD_SIGNAL(MethodInfo("page_changed", PropertyInfo(Variant::STRING, "page")));
	ADD_SIGNAL(MethodInfo("layouts_changed"));
}

StudioPagesBar::StudioPagesBar() {
	set_name("StudioPagesBar");
	config_path = EditorSettings::get_singleton() ? EditorSettings::get_singleton()->get_editor_layouts_config() : String();

	button_group.instantiate();
	button_group->set_allow_unpress(true);

	add_button = memnew(Button);
	add_button->set_text("+");
	add_button->set_theme_type_variation("FlatButton");
	add_button->set_focus_mode(FOCUS_ACCESSIBILITY);
	add_button->set_tooltip_text(TTR("Save the current dock layout as a new page."));
	add_button->set_accessibility_name(TTRC("New Layout Page"));
	add_button->connect(SceneStringName(pressed), callable_mp(this, &StudioPagesBar::_add_button_pressed));
	add_child(add_button);

	context_menu = memnew(PopupMenu);
	context_menu->add_item(TTR("Save Current Layout Here"), CONTEXT_SAVE_HERE);
	context_menu->add_item(TTR("Rename..."), CONTEXT_RENAME);
	context_menu->add_separator();
	context_menu->add_item(TTR("Delete"), CONTEXT_DELETE);
	context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &StudioPagesBar::_context_option));
	add_child(context_menu);

	name_dialog = memnew(ConfirmationDialog);
	VBoxContainer *vb = memnew(VBoxContainer);
	name_dialog->add_child(vb);
	name_edit = memnew(LineEdit);
	name_edit->set_accessibility_name(TTRC("Page name"));
	name_edit->connect(SceneStringName(text_changed), callable_mp(this, &StudioPagesBar::_name_text_changed));
	vb->add_child(name_edit);
	name_error = memnew(Label);
	name_error->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	name_error->hide();
	vb->add_child(name_error);
	name_dialog->register_text_enter(name_edit);
	name_dialog->connect(SceneStringName(confirmed), callable_mp(this, &StudioPagesBar::_name_confirmed));
	add_child(name_dialog);
}
