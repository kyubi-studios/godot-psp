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

#include "editor/studio/studio_scene_preview.h"

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

} // namespace TestStudioScenePreview

#endif // TOOLS_ENABLED
