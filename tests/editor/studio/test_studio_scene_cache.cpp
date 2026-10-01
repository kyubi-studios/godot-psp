/**************************************************************************/
/*  test_studio_scene_cache.cpp                                           */
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

TEST_FORCE_LINK(test_studio_scene_cache)

#ifdef TOOLS_ENABLED

#include "core/io/resource.h"
#include "editor/studio/studio_scene_cache.h"

namespace TestStudioSceneCache {

static Ref<Resource> res() {
	Ref<Resource> r;
	r.instantiate();
	return r;
}

TEST_CASE("[Studio] Scene cache evicts the oldest entry beyond capacity") {
	StudioSceneCache cache;
	cache.set_capacity(2);
	cache.put("res://a.tscn", res());
	cache.put("res://b.tscn", res());
	cache.put("res://c.tscn", res());
	CHECK(cache.size() == 2);
	CHECK_FALSE(cache.has("res://a.tscn"));
	PackedStringArray paths = cache.get_paths();
	REQUIRE(paths.size() == 2);
	CHECK(paths[0] == "res://c.tscn");
	CHECK(paths[1] == "res://b.tscn");
}

TEST_CASE("[Studio] Re-adding a cached scene moves it to the front") {
	StudioSceneCache cache;
	cache.set_capacity(3);
	cache.put("res://a.tscn", res());
	cache.put("res://b.tscn", res());
	cache.put("res://a.tscn", res());
	CHECK(cache.size() == 2);
	CHECK(cache.get_paths()[0] == "res://a.tscn");
}

TEST_CASE("[Studio] Zero capacity disables and clears the cache") {
	StudioSceneCache cache;
	cache.set_capacity(3);
	cache.put("res://a.tscn", res());
	cache.set_capacity(0);
	CHECK(cache.size() == 0);
	cache.put("res://b.tscn", res());
	CHECK(cache.size() == 0);
}

TEST_CASE("[Studio] Shrinking the capacity drops the oldest entries") {
	StudioSceneCache cache;
	cache.set_capacity(4);
	cache.put("res://a.tscn", res());
	cache.put("res://b.tscn", res());
	cache.put("res://c.tscn", res());
	cache.set_capacity(1);
	CHECK(cache.size() == 1);
	CHECK(cache.has("res://c.tscn"));
}

TEST_CASE("[Studio] Null resources and empty paths are ignored") {
	StudioSceneCache cache;
	cache.set_capacity(2);
	cache.put("res://a.tscn", Ref<Resource>());
	cache.put("", res());
	CHECK(cache.size() == 0);
}

TEST_CASE("[Studio] Evicted and erased resources are released") {
	StudioSceneCache cache;
	cache.set_capacity(1);
	Ref<Resource> a = res();
	cache.put("res://a.tscn", a);
	CHECK(a->get_reference_count() == 2);
	cache.put("res://b.tscn", res());
	CHECK(a->get_reference_count() == 1);

	Ref<Resource> b = res();
	cache.put("res://b.tscn", b);
	cache.erase("res://b.tscn");
	CHECK(b->get_reference_count() == 1);
	CHECK(cache.size() == 0);
}

} // namespace TestStudioSceneCache

#endif // TOOLS_ENABLED
