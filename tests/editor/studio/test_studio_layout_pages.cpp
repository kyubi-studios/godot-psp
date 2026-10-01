/**************************************************************************/
/*  test_studio_layout_pages.cpp                                          */
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

TEST_FORCE_LINK(test_studio_layout_pages)

#ifdef TOOLS_ENABLED

#include "core/io/config_file.h"
#include "editor/studio/studio_layout_pages.h"

namespace TestStudioLayoutPages {

static Ref<ConfigFile> make_layouts() {
	Ref<ConfigFile> config;
	config.instantiate();
	config->set_value("Anim", "dock_1", "Scene,Import");
	config->set_value("Anim/window", "size", 10);
	config->set_value("Animation", "dock_2", "Inspector");
	config->set_value("Code", "dock_3", "FileSystem");
	return config;
}

TEST_CASE("[Studio] Layout page names are validated") {
	CHECK(StudioLayoutPages::validate_page_name("Anim").is_empty());
	CHECK(StudioLayoutPages::validate_page_name("  Level Design ").is_empty());
	CHECK_FALSE(StudioLayoutPages::validate_page_name("").is_empty());
	CHECK_FALSE(StudioLayoutPages::validate_page_name("   ").is_empty());
	CHECK_FALSE(StudioLayoutPages::validate_page_name("a/b").is_empty());
	CHECK_FALSE(StudioLayoutPages::validate_page_name("a\\b").is_empty());
}

TEST_CASE("[Studio] Layout pages are listed in config order without subsections") {
	PackedStringArray pages = StudioLayoutPages::list_pages(make_layouts());
	REQUIRE(pages.size() == 3);
	CHECK(pages[0] == "Anim");
	CHECK(pages[1] == "Animation");
	CHECK(pages[2] == "Code");
}

TEST_CASE("[Studio] Listing pages of a null config is empty") {
	CHECK(StudioLayoutPages::list_pages(Ref<ConfigFile>()).is_empty());
}

TEST_CASE("[Studio] Erasing a page removes its subsections but not pages sharing a prefix") {
	Ref<ConfigFile> config = make_layouts();
	StudioLayoutPages::erase_page(config, "Anim");
	CHECK_FALSE(config->has_section("Anim"));
	CHECK_FALSE(config->has_section("Anim/window"));
	CHECK(config->has_section("Animation"));
	CHECK(config->has_section("Code"));
}

TEST_CASE("[Studio] Renaming a page moves all keys and subsections") {
	Ref<ConfigFile> config = make_layouts();
	CHECK(StudioLayoutPages::rename_page(config, "Anim", "Rigging") == OK);
	CHECK_FALSE(config->has_section("Anim"));
	CHECK_FALSE(config->has_section("Anim/window"));
	CHECK(String(config->get_value("Rigging", "dock_1")) == "Scene,Import");
	CHECK(int(config->get_value("Rigging/window", "size")) == 10);
	CHECK(config->has_section("Animation"));
}

TEST_CASE("[Studio] Renaming onto an existing page is refused and keeps the source") {
	Ref<ConfigFile> config = make_layouts();
	CHECK(StudioLayoutPages::rename_page(config, "Anim", "Code") == ERR_ALREADY_EXISTS);
	CHECK(String(config->get_value("Anim", "dock_1")) == "Scene,Import");
	CHECK(String(config->get_value("Code", "dock_3")) == "FileSystem");
}

TEST_CASE("[Studio] Renaming a missing page or to an invalid name fails") {
	Ref<ConfigFile> config = make_layouts();
	CHECK(StudioLayoutPages::rename_page(config, "Nope", "Other") == ERR_DOES_NOT_EXIST);
	CHECK(StudioLayoutPages::rename_page(config, "Anim", "a/b") == ERR_INVALID_PARAMETER);
	CHECK(config->has_section("Anim"));
}

TEST_CASE("[Studio] Renaming a page to its own name is a no-op") {
	Ref<ConfigFile> config = make_layouts();
	CHECK(StudioLayoutPages::rename_page(config, "Anim", "Anim") == OK);
	CHECK(String(config->get_value("Anim", "dock_1")) == "Scene,Import");
	CHECK(config->has_section("Anim/window"));
}

TEST_CASE("[Studio] Copying a page duplicates keys and subsections") {
	Ref<ConfigFile> config = make_layouts();
	CHECK(StudioLayoutPages::copy_page(config, "Anim", "Anim 2") == OK);
	CHECK(String(config->get_value("Anim 2", "dock_1")) == "Scene,Import");
	CHECK(int(config->get_value("Anim 2/window", "size")) == 10);
	CHECK(config->has_section("Anim"));
}

} // namespace TestStudioLayoutPages

#endif // TOOLS_ENABLED
