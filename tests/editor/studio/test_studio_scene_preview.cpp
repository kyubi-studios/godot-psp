/**************************************************************************/
/*  test_studio_scene_preview.cpp                                         */
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

TEST_FORCE_LINK(test_studio_scene_preview)

#ifdef TOOLS_ENABLED

#include "core/io/file_access.h"
#include "editor/studio/studio_scene_preview.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/node_3d.h"
#include "tests/test_tools.h"
#include "tests/test_utils.h"

namespace TestStudioScenePreview {

TEST_CASE("[Studio] Orbit camera looks at the center of the bounds from a sensible distance") {
	const AABB bounds(Vector3(-1, 0, -1), Vector3(2, 2, 2));
	const Transform3D xf = StudioScenePreview::orbit_transform(bounds, 0.5, -0.4, 1.5);
	const Vector3 center = bounds.get_center();
	const real_t distance = xf.origin.distance_to(center);
	CHECK(distance == doctest::Approx(bounds.size.length() * 1.5).epsilon(0.001));
	// Camera looks down -Z: the forward vector must point at the center.
	const Vector3 forward = -xf.basis.get_column(2).normalized();
	CHECK(forward.is_equal_approx((center - xf.origin).normalized()));
	// Negative pitch places the camera above the target.
	CHECK(xf.origin.y > center.y);
}

TEST_CASE("[Studio] Orbit camera has a valid default for empty bounds") {
	const Transform3D xf = StudioScenePreview::orbit_transform(AABB(), 0.0, -0.3, 1.0);
	CHECK(xf.origin.is_finite());
	CHECK(xf.basis.is_finite());
	CHECK(xf.origin.length() >= 0.5);
}

TEST_CASE("[Studio] Canvas fit centers and scales the content") {
	const Rect2 content(Vector2(100, 100), Vector2(200, 100));
	const Size2 viewport(400, 400);
	const Transform2D xf = StudioScenePreview::fit_canvas_transform(content, viewport, 1.0, Vector2());
	const Vector2 center = xf.xform(content.get_center());
	CHECK(center.is_equal_approx(viewport / 2));
	// Width is the limiting side: 200 * scale = 90% of 400.
	CHECK(xf.get_scale().x == doctest::Approx(1.8));
}

TEST_CASE("[Studio] Canvas fit applies zoom and pan") {
	const Rect2 content(Vector2(0, 0), Vector2(100, 100));
	const Transform2D base = StudioScenePreview::fit_canvas_transform(content, Size2(200, 200), 1.0, Vector2());
	const Transform2D zoomed = StudioScenePreview::fit_canvas_transform(content, Size2(200, 200), 2.0, Vector2(10, -5));
	CHECK(zoomed.get_scale().x == doctest::Approx(base.get_scale().x * 2.0));
	CHECK(zoomed.xform(content.get_center()).is_equal_approx(Vector2(110, 95)));
}

TEST_CASE("[Studio] Canvas fit handles empty content and zero-size viewports") {
	const Transform2D a = StudioScenePreview::fit_canvas_transform(Rect2(), Size2(200, 100), 1.0, Vector2());
	CHECK(a.get_scale().x == doctest::Approx(1.0));
	CHECK(a.get_origin().is_finite());
	const Transform2D b = StudioScenePreview::fit_canvas_transform(Rect2(0, 0, 10, 10), Size2(), 1.0, Vector2());
	CHECK(b.get_origin().is_finite());
	CHECK(Math::is_finite(b.get_scale().x));
}

static String write_scene(const String &p_name, const String &p_text) {
	const String path = TestUtils::get_temp_path(p_name);
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
	REQUIRE(f.is_valid());
	f->store_string(p_text);
	return path;
}

static const char *SCENE_3D = "[gd_scene format=3]\n\n[node name=\"Level\" type=\"Node3D\"]\n\n[node name=\"Box\" type=\"CSGBox3D\" parent=\".\"]\n";
static const char *SCENE_2D = "[gd_scene format=3]\n\n[node name=\"Menu\" type=\"Node2D\"]\n\n[node name=\"Child\" type=\"Node2D\" parent=\".\"]\nposition = Vector2(100, 50)\n";

TEST_CASE("[Editor][Studio] Scene preview instantiates a 3D scene") {
	StudioScenePreview *preview = memnew(StudioScenePreview);
	preview->set_scene_path(write_scene("studio_preview_3d.tscn", SCENE_3D));
	preview->refresh();
	CHECK(preview->get_error().is_empty());
	REQUIRE(preview->get_preview_root() != nullptr);
	CHECK(Object::cast_to<Node3D>(preview->get_preview_root()) != nullptr);
	CHECK(preview->is_3d());
	memdelete(preview);
}

TEST_CASE("[Editor][Studio] Scene preview instantiates a 2D scene") {
	StudioScenePreview *preview = memnew(StudioScenePreview);
	preview->set_scene_path(write_scene("studio_preview_2d.tscn", SCENE_2D));
	ErrorDetector errors;
	preview->refresh();
	CHECK_FALSE(errors.has_error);
	REQUIRE(preview->get_preview_root() != nullptr);
	CHECK(Object::cast_to<Node2D>(preview->get_preview_root()) != nullptr);
	CHECK_FALSE(preview->is_3d());
	memdelete(preview);
}

TEST_CASE("[Editor][Studio] Refreshing the preview frees the previous instance") {
	StudioScenePreview *preview = memnew(StudioScenePreview);
	preview->set_scene_path(write_scene("studio_preview_3d.tscn", SCENE_3D));
	preview->refresh();
	REQUIRE(preview->get_preview_root() != nullptr);
	const ObjectID old_id = preview->get_preview_root()->get_instance_id();
	preview->refresh();
	CHECK(ObjectDB::get_instance(old_id) == nullptr);
	CHECK(preview->get_preview_root() != nullptr);
	memdelete(preview);
}

TEST_CASE("[Editor][Studio] Previewing a missing or broken file shows an error") {
	StudioScenePreview *preview = memnew(StudioScenePreview);
	ERR_PRINT_OFF;
	preview->set_scene_path(TestUtils::get_temp_path("studio_preview_missing.tscn"));
	preview->refresh();
	CHECK_FALSE(preview->get_error().is_empty());
	CHECK(preview->get_preview_root() == nullptr);

	preview->set_scene_path(write_scene("studio_preview_broken.tscn", "this is not a scene"));
	preview->refresh();
	ERR_PRINT_ON;
	CHECK_FALSE(preview->get_error().is_empty());
	CHECK(preview->get_preview_root() == nullptr);
	memdelete(preview);
}

} // namespace TestStudioScenePreview

#endif // TOOLS_ENABLED
