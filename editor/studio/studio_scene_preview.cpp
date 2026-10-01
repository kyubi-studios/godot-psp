/**************************************************************************/
/*  studio_scene_preview.cpp                                              */
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


#include "studio_scene_preview.h"

Transform3D StudioScenePreview::orbit_transform(const AABB &p_bounds, float p_yaw, float p_pitch, float p_distance_scale) {
	const Vector3 target = p_bounds.has_volume() || p_bounds.size != Vector3() ? p_bounds.get_center() : Vector3();
	const real_t distance = MAX(0.5, p_bounds.size.length() * p_distance_scale);
	const Vector3 offset = Vector3(Math::sin(p_yaw) * Math::cos(p_pitch), -Math::sin(p_pitch), Math::cos(p_yaw) * Math::cos(p_pitch)) * distance;
	Transform3D xf;
	xf.origin = target + offset;
	xf.basis = Basis::looking_at(target - xf.origin, Vector3(0, 1, 0));
	return xf;
}

Transform2D StudioScenePreview::fit_canvas_transform(const Rect2 &p_content, const Size2 &p_viewport, float p_zoom, const Vector2 &p_pan) {
	real_t scale = 1.0;
	if (p_content.has_area() && p_viewport.x > 0 && p_viewport.y > 0) {
		scale = MIN(p_viewport.x * 0.9 / p_content.size.x, p_viewport.y * 0.9 / p_content.size.y);
	}
	scale *= p_zoom;
	const Vector2 origin = p_viewport / 2 - p_content.get_center() * scale + p_pan;
	return Transform2D(0, Size2(scale, scale), 0, origin);
}

StudioScenePreview::StudioScenePreview() {
	set_name("StudioScenePreview");
}
