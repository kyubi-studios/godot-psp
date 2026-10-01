/**************************************************************************/
/*  studio_theme.cpp                                                      */
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


#include "studio_theme.h"

#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme.h"
#include "scene/resources/style_box_flat.h"
#include "scene/scene_string_names.h"

Color StudioThemeColors::base_color(float p_hue, float p_vividness) {
	const float hue = CLAMP(p_hue, 0.0f, 1.0f);
	const float vividness = CLAMP(p_vividness, 0.0f, 1.0f);
	const float saturation = vividness > 0.0f ? 0.15f + 0.4f * vividness : 0.0f;
	return Color::from_hsv(hue, saturation, 0.21f);
}

Color StudioThemeColors::accent_color(float p_hue, float p_vividness) {
	const float hue = CLAMP(p_hue, 0.0f, 1.0f);
	const float vividness = CLAMP(p_vividness, 0.0f, 1.0f);
	return Color::from_hsv(hue, 0.45f + 0.35f * vividness, 0.85f);
}

void StudioTheme::get_preset_colors(Color &r_base, Color &r_accent, float &r_contrast) {
	const float vividness = EDITOR_GET("interface/theme/studio/vividness");
	r_base = StudioThemeColors::base_color(EDITOR_GET("interface/theme/studio/base_hue"), vividness);
	r_accent = StudioThemeColors::accent_color(EDITOR_GET("interface/theme/studio/accent_hue"), vividness);
	r_contrast = 0.25;
}

// Floating surfaces (menus, popups, tooltips): translucent, rounded, with a light rim and a soft shadow.
static Ref<StyleBoxFlat> _make_floating(const Ref<StyleBox> &p_base, const EditorThemeManager::ThemeConfiguration &p_config) {
	Ref<StyleBoxFlat> base = p_base;
	Ref<StyleBoxFlat> style = base.is_valid() ? Ref<StyleBoxFlat>(base->duplicate()) : Ref<StyleBoxFlat>(memnew(StyleBoxFlat));
	Color bg = style->get_bg_color();
	bg.a = StudioTheme::POPUP_ALPHA;
	style->set_bg_color(bg);
	style->set_corner_radius_all((StudioTheme::CORNER_RADIUS + 2) * EDSCALE);
	style->set_border_width_all(MAX(1, Math::round(EDSCALE)));
	style->set_border_color(p_config.mono_color * Color(1, 1, 1, 0.12));
	style->set_shadow_color(Color(0, 0, 0, 0.45));
	style->set_shadow_size(10 * EDSCALE);
	style->set_shadow_offset(Vector2(0, 3) * EDSCALE);
	style->set_anti_aliased(true);
	return style;
}

static Ref<StyleBoxFlat> _duplicate_flat(const Ref<StyleBox> &p_style) {
	Ref<StyleBoxFlat> flat = p_style;
	return flat.is_valid() ? Ref<StyleBoxFlat>(flat->duplicate()) : Ref<StyleBoxFlat>(memnew(StyleBoxFlat));
}

// Dock and editor tabs: a darker header band, with an accent line on top of the selected tab.
static void _populate_tabs(const Ref<EditorTheme> &p_theme, const EditorThemeManager::ThemeConfiguration &p_config) {
	Ref<StyleBoxFlat> band = memnew(StyleBoxFlat);
	band->set_bg_color(p_config.dark_color_2);
	band->set_corner_radius(CORNER_TOP_LEFT, StudioTheme::CORNER_RADIUS * EDSCALE);
	band->set_corner_radius(CORNER_TOP_RIGHT, StudioTheme::CORNER_RADIUS * EDSCALE);
	p_theme->set_stylebox("tabbar_background", "TabContainer", band);

	Ref<StyleBoxFlat> selected = _duplicate_flat(p_theme->get_stylebox("tab_selected", "TabContainer"));
	selected->set_border_width(SIDE_TOP, MAX(2, Math::round(2 * EDSCALE)));
	selected->set_border_color(p_config.accent_color);
	p_theme->set_stylebox("tab_selected", "TabContainer", selected);
	p_theme->set_stylebox("tab_selected", "TabBar", selected);
}

// Selected rows in trees and lists: a translucent accent fill instead of a thin outline.
static void _populate_selection(const Ref<EditorTheme> &p_theme, const EditorThemeManager::ThemeConfiguration &p_config) {
	for (const char *type : { "Tree", "ItemList" }) {
		for (const char *state : { "selected", "selected_focus" }) {
			Ref<StyleBoxFlat> style = _duplicate_flat(p_theme->get_stylebox(state, type));
			Color fill = p_config.accent_color;
			fill.a = String(state) == "selected_focus" ? 0.38 : 0.24;
			style->set_bg_color(fill);
			style->set_border_width_all(0);
			style->set_corner_radius_all(3 * EDSCALE);
			style->set_draw_center(true);
			p_theme->set_stylebox(state, type, style);
		}
	}
}

void StudioTheme::populate_overrides(const Ref<EditorTheme> &p_theme, const EditorThemeManager::ThemeConfiguration &p_config) {
	for (const char *type : { "PopupMenu", "PopupPanel", "TooltipPanel" }) {
		p_theme->set_stylebox(SceneStringName(panel), type, _make_floating(p_theme->get_stylebox(SceneStringName(panel), type), p_config));
	}
	_populate_tabs(p_theme, p_config);
	_populate_selection(p_theme, p_config);
}
