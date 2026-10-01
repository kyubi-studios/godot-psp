/**************************************************************************/
/*  studio_recent_button.cpp                                              */
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


#include "studio_recent_button.h"

#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/popup_menu.h"

PackedStringArray StudioRecentButton::collect(const Array &p_scenes, const Array &p_scripts, const Callable &p_exists, int p_max) {
	PackedStringArray result;
	for (const Array &list : { p_scenes, p_scripts }) {
		for (const Variant &entry : list) {
			if (result.size() >= p_max) {
				return result;
			}
			if (entry.get_type() != Variant::STRING) {
				continue;
			}
			const String path = entry;
			if (path.is_empty() || result.has(path) || !bool(p_exists.call(path))) {
				continue;
			}
			result.push_back(path);
		}
	}
	return result;
}

static bool _resource_exists(const String &p_path) {
	return ResourceLoader::exists(p_path);
}

void StudioRecentButton::_about_to_popup() {
	EditorSettings *settings = EditorSettings::get_singleton();
	items = collect(settings->get_project_metadata("recent_files", "scenes", Array()),
			settings->get_project_metadata("recent_files", "scripts", Array()),
			callable_mp_static(&_resource_exists), MAX_ITEMS);

	PopupMenu *popup = get_popup();
	popup->clear();
	if (items.is_empty()) {
		popup->add_item(TTR("No recent files"));
		popup->set_item_disabled(-1, true);
		return;
	}
	for (int i = 0; i < items.size(); i++) {
		const bool is_scene = ResourceLoader::get_resource_type(items[i]) == "PackedScene";
		popup->add_icon_item(get_editor_theme_icon(is_scene ? SNAME("PackedScene") : SNAME("Script")), items[i].trim_prefix("res://"), i);
		popup->set_item_tooltip(-1, items[i]);
	}
}

void StudioRecentButton::_item_pressed(int p_index) {
	ERR_FAIL_INDEX(p_index, items.size());
	const String path = items[p_index];
	if (ResourceLoader::get_resource_type(path) == "PackedScene") {
		EditorNode::get_singleton()->load_scene(path);
	} else {
		EditorNode::get_singleton()->load_resource(path);
	}
}

void StudioRecentButton::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		set_button_icon(get_editor_theme_icon(SNAME("History")));
	}
}

StudioRecentButton::StudioRecentButton() {
	set_name("StudioRecentButton");
	set_flat(true);
	set_theme_type_variation("FlatMenuButton");
	set_focus_mode(FOCUS_ACCESSIBILITY);
	set_switch_on_hover(true);
	set_tooltip_text(TTRC("Recent scenes and scripts"));
	set_accessibility_name(TTRC("Recent Files"));
	get_popup()->connect("about_to_popup", callable_mp(this, &StudioRecentButton::_about_to_popup));
	get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &StudioRecentButton::_item_pressed));
}
