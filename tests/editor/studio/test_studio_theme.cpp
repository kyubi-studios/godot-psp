/**************************************************************************/
/*  test_studio_theme.cpp                                                 */
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


#include "tests/test_macros.h"

TEST_FORCE_LINK(test_studio_theme)

#ifdef TOOLS_ENABLED

#include "editor/settings/editor_settings.h"
#include "editor/studio/studio_theme.h"
#include "editor/themes/editor_theme.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/resources/style_box_flat.h"
#include "scene/scene_string_names.h"

namespace TestStudioTheme {

TEST_CASE("[Studio] Base color follows the hue and stays dark") {
	const Color blue = StudioThemeColors::base_color(0.62, 0.3);
	CHECK(blue.b > blue.r);
	CHECK(blue.b > blue.g);
	CHECK(blue.get_luminance() < 0.25);
	CHECK(blue.a == 1.0);
}

TEST_CASE("[Studio] Zero vividness gives a neutral gray base") {
	const Color gray = StudioThemeColors::base_color(0.3, 0.0);
	CHECK(gray.r == doctest::Approx(gray.g));
	CHECK(gray.g == doctest::Approx(gray.b));
}

TEST_CASE("[Studio] Accent color is bright and saturated") {
	const Color accent = StudioThemeColors::accent_color(0.25, 0.3);
	CHECK(accent.get_luminance() > 0.35);
	CHECK(accent.get_s() > 0.4);
	CHECK(accent.g > accent.b);
}

TEST_CASE("[Studio] Hue wraps and inputs are clamped") {
	const Color a = StudioThemeColors::accent_color(0.0, 0.5);
	const Color b = StudioThemeColors::accent_color(1.0, 0.5);
	CHECK(a.is_equal_approx(b));
	const Color c = StudioThemeColors::base_color(-3.0, 9.0);
	CHECK(c.r >= 0.0);
	CHECK(c.r <= 1.0);
	CHECK(c.g >= 0.0);
	CHECK(c.b <= 1.0);
	CHECK(c.is_equal_approx(StudioThemeColors::base_color(0.0, 1.0)));
}

static Ref<EditorTheme> generate_with(const String &p_style, const String &p_preset) {
	EditorSettings::get_singleton()->set("interface/theme/style", p_style);
	EditorSettings::get_singleton()->set("interface/theme/color_preset", p_preset);
	return EditorThemeManager::generate_theme();
}

TEST_CASE("[Editor][Studio] Studio style makes popup menus translucent and rounded") {
	Ref<EditorTheme> theme = generate_with("Studio", "Studio");
	Ref<StyleBoxFlat> panel = theme->get_stylebox(SceneStringName(panel), "PopupMenu");
	REQUIRE(panel.is_valid());
	CHECK(panel->get_bg_color().a < 1.0);
	CHECK(panel->get_corner_radius(CORNER_TOP_LEFT) > 0);
}

TEST_CASE("[Editor][Studio] Studio dock tabs have an accent line on the selected tab") {
	Ref<EditorTheme> theme = generate_with("Studio", "Studio");
	const Color accent = EDITOR_GET("interface/theme/accent_color");
	Ref<StyleBoxFlat> selected = theme->get_stylebox("tab_selected", "TabContainer");
	REQUIRE(selected.is_valid());
	CHECK(selected->get_border_width(SIDE_TOP) > 0);
	CHECK(selected->get_border_color().is_equal_approx(accent));
	Ref<StyleBoxFlat> band = theme->get_stylebox("tabbar_background", "TabContainer");
	REQUIRE(band.is_valid());
	CHECK(band->get_bg_color().a > 0.0);
}

TEST_CASE("[Editor][Studio] Studio tree selection is filled with the accent color") {
	Ref<EditorTheme> theme = generate_with("Studio", "Studio");
	const Color accent = EDITOR_GET("interface/theme/accent_color");
	Ref<StyleBoxFlat> selected = theme->get_stylebox("selected_focus", "Tree");
	REQUIRE(selected.is_valid());
	const Color bg = selected->get_bg_color();
	CHECK(Color(bg.r, bg.g, bg.b).is_equal_approx(Color(accent.r, accent.g, accent.b)));
	CHECK(bg.a > 0.2);
	CHECK(bg.a < 0.6);
}

TEST_CASE("[Editor][Studio] Modern style popup menus are unchanged (opaque)") {
	Ref<EditorTheme> theme = generate_with("Modern", "Default");
	Ref<StyleBoxFlat> panel = theme->get_stylebox(SceneStringName(panel), "PopupMenu");
	REQUIRE(panel.is_valid());
	CHECK(panel->get_bg_color().a == 1.0);
}

TEST_CASE("[Editor][Studio] Studio color preset derives colors from the hue settings") {
	EditorSettings::get_singleton()->set("interface/theme/studio/base_hue", 0.62);
	EditorSettings::get_singleton()->set("interface/theme/studio/accent_hue", 0.25);
	EditorSettings::get_singleton()->set("interface/theme/studio/vividness", 0.3);
	generate_with("Studio", "Studio");
	const Color base = EDITOR_GET("interface/theme/base_color");
	const Color accent = EDITOR_GET("interface/theme/accent_color");
	CHECK(base.is_equal_approx(StudioThemeColors::base_color(0.62, 0.3)));
	CHECK(accent.is_equal_approx(StudioThemeColors::accent_color(0.25, 0.3)));
}

TEST_CASE("[Editor][Studio] Custom color preset is not overridden by Studio hue settings") {
	EditorSettings::get_singleton()->set("interface/theme/base_color", Color(0.3, 0.1, 0.1));
	generate_with("Studio", "Custom");
	const Color base = EDITOR_GET("interface/theme/base_color");
	CHECK(base.is_equal_approx(Color(0.3, 0.1, 0.1)));
}

} // namespace TestStudioTheme

#endif // TOOLS_ENABLED
