/**************************************************************************/
/*  studio_scene_cache.h                                                  */
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


#pragma once

#include "core/io/resource.h"
#include "core/templates/local_vector.h"

// Keeps the most recently closed scenes loaded (newest first), so their dependencies stay in
// the resource cache and reopening them skips loading textures, meshes and other resources.
class StudioSceneCache {
	struct Entry {
		String path;
		Ref<Resource> resource;
	};

	LocalVector<Entry> entries;
	int capacity = 0;

	int _find(const String &p_path) const;
	void _trim();

public:
	void set_capacity(int p_capacity);
	int get_capacity() const { return capacity; }

	void put(const String &p_path, const Ref<Resource> &p_resource);
	bool has(const String &p_path) const;
	void erase(const String &p_path);
	void clear();

	PackedStringArray get_paths() const;
	int size() const { return entries.size(); }
};
