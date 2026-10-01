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

#include "editor/studio/studio_theme.h"

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

} // namespace TestStudioTheme

#endif // TOOLS_ENABLED
