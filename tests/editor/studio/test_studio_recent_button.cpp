/**************************************************************************/
/*  test_studio_recent_button.cpp                                         */
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

TEST_FORCE_LINK(test_studio_recent_button)

#ifdef TOOLS_ENABLED

#include "core/object/callable_mp.h"
#include "editor/studio/studio_recent_button.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestStudioRecentButton {

static bool _exists_unless_deleted(const String &p_path) {
	return !p_path.contains("deleted");
}

static PackedStringArray collect(const Array &p_scenes, const Array &p_scripts, int p_max) {
	return StudioRecentButton::collect(p_scenes, p_scripts, callable_mp_static(&_exists_unless_deleted), p_max);
}

TEST_CASE("[Studio] Recent files list scenes first, then scripts") {
	Array scenes = { "res://a.tscn", "res://b.tscn" };
	Array scripts = { "res://c.gd" };
	PackedStringArray list = collect(scenes, scripts, 10);
	REQUIRE(list.size() == 3);
	CHECK(list[0] == "res://a.tscn");
	CHECK(list[1] == "res://b.tscn");
	CHECK(list[2] == "res://c.gd");
}

TEST_CASE("[Studio] Recent files skip duplicates and missing files") {
	Array scenes = { "res://a.tscn", "res://deleted.tscn", "res://a.tscn" };
	Array scripts = { "res://deleted.gd", "res://c.gd" };
	PackedStringArray list = collect(scenes, scripts, 10);
	REQUIRE(list.size() == 2);
	CHECK(list[0] == "res://a.tscn");
	CHECK(list[1] == "res://c.gd");
}

TEST_CASE("[Studio] Recent files respect the maximum and empty inputs") {
	Array scenes = { "res://1.tscn", "res://2.tscn", "res://3.tscn" };
	CHECK(collect(scenes, Array(), 2).size() == 2);
	CHECK(collect(Array(), Array(), 10).is_empty());
	CHECK(collect(scenes, Array(), 0).is_empty());
}

TEST_CASE("[Studio] Recent files ignore non-string entries") {
	Array scenes = { 42, "res://a.tscn", Variant() };
	PackedStringArray list = collect(scenes, Array(), 10);
	REQUIRE(list.size() == 1);
	CHECK(list[0] == "res://a.tscn");
}

TEST_CASE("[Editor][Studio] Recent files button gets its icon when entering the tree") {
	StudioRecentButton *button = memnew(StudioRecentButton);
	SceneTree::get_singleton()->get_root()->add_child(button);
	CHECK(button->get_button_icon().is_valid());
	memdelete(button);
}

} // namespace TestStudioRecentButton

#endif // TOOLS_ENABLED
