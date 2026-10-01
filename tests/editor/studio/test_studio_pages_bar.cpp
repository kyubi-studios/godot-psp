/**************************************************************************/
/*  test_studio_pages_bar.cpp                                             */
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

TEST_FORCE_LINK(test_studio_pages_bar)

#ifdef TOOLS_ENABLED

#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "editor/studio/studio_pages_bar.h"
#include "scene/gui/button.h"
#include "tests/test_utils.h"

namespace TestStudioPagesBar {

static String write_layouts() {
	const String path = TestUtils::get_temp_path("studio_pages_bar_layouts.cfg");
	Ref<ConfigFile> config;
	config.instantiate();
	config->set_value("Anim", "dock_1", "Scene");
	config->set_value("Anim/window", "size", 1);
	config->set_value("Code", "dock_3", "FileSystem");
	REQUIRE(config->save(path) == OK);
	return path;
}

static int count_page_buttons(StudioPagesBar *p_bar) {
	int count = 0;
	for (int i = 0; i < p_bar->get_child_count(); i++) {
		Button *button = Object::cast_to<Button>(p_bar->get_child(i));
		if (button && button->is_toggle_mode()) {
			count++;
		}
	}
	return count;
}

TEST_CASE("[Editor][Studio] Pages bar shows one toggle button per layout page") {
	StudioPagesBar *bar = memnew(StudioPagesBar);
	bar->set_config_path(write_layouts());
	bar->refresh();
	CHECK(count_page_buttons(bar) == 2);
	memdelete(bar);
}

TEST_CASE("[Editor][Studio] Pages bar with a missing layouts file shows no pages") {
	StudioPagesBar *bar = memnew(StudioPagesBar);
	bar->set_config_path(TestUtils::get_temp_path("studio_pages_bar_missing.cfg"));
	bar->refresh();
	CHECK(count_page_buttons(bar) == 0);
	memdelete(bar);
}

TEST_CASE("[Editor][Studio] Renaming the current page keeps it current and persists") {
	const String path = write_layouts();
	StudioPagesBar *bar = memnew(StudioPagesBar);
	bar->set_config_path(path);
	bar->set_current_page("Anim");

	CHECK(bar->rename_page("Anim", "Rigging") == OK);
	CHECK(bar->get_current_page() == "Rigging");

	Ref<ConfigFile> saved;
	saved.instantiate();
	REQUIRE(saved->load(path) == OK);
	CHECK(saved->has_section("Rigging"));
	CHECK(saved->has_section("Rigging/window"));
	CHECK_FALSE(saved->has_section("Anim"));
	memdelete(bar);
}

TEST_CASE("[Editor][Studio] Renaming onto an existing page fails and changes nothing") {
	const String path = write_layouts();
	StudioPagesBar *bar = memnew(StudioPagesBar);
	bar->set_config_path(path);
	bar->set_current_page("Anim");

	CHECK(bar->rename_page("Anim", "Code") == ERR_ALREADY_EXISTS);
	CHECK(bar->get_current_page() == "Anim");

	Ref<ConfigFile> saved;
	saved.instantiate();
	REQUIRE(saved->load(path) == OK);
	CHECK(saved->has_section("Anim"));
	memdelete(bar);
}

TEST_CASE("[Editor][Studio] Deleting the current page clears the current page") {
	const String path = write_layouts();
	StudioPagesBar *bar = memnew(StudioPagesBar);
	bar->set_config_path(path);
	bar->set_current_page("Code");

	bar->delete_page("Code");
	CHECK(bar->get_current_page().is_empty());
	CHECK(count_page_buttons(bar) == 1);

	Ref<ConfigFile> saved;
	saved.instantiate();
	REQUIRE(saved->load(path) == OK);
	CHECK_FALSE(saved->has_section("Code"));
	memdelete(bar);
}

TEST_CASE("[Editor][Studio] Switching to a page index outside the list is ignored") {
	StudioPagesBar *bar = memnew(StudioPagesBar);
	bar->set_config_path(write_layouts());
	bar->set_current_page("Anim");
	bar->switch_to_index(5);
	bar->switch_to_index(-1);
	CHECK(bar->get_current_page() == "Anim");
	memdelete(bar);
}

} // namespace TestStudioPagesBar

#endif // TOOLS_ENABLED
