/**************************************************************************/
/*  studio_scene_cache.cpp                                                */
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


#include "studio_scene_cache.h"

int StudioSceneCache::_find(const String &p_path) const {
	for (uint32_t i = 0; i < entries.size(); i++) {
		if (entries[i].path == p_path) {
			return i;
		}
	}
	return -1;
}

void StudioSceneCache::_trim() {
	while ((int)entries.size() > capacity) {
		entries.remove_at(entries.size() - 1);
	}
}

void StudioSceneCache::set_capacity(int p_capacity) {
	capacity = MAX(0, p_capacity);
	_trim();
}

void StudioSceneCache::put(const String &p_path, const Ref<Resource> &p_resource) {
	if (capacity == 0 || p_path.is_empty() || p_resource.is_null()) {
		return;
	}
	erase(p_path);
	entries.insert(0, Entry{ p_path, p_resource });
	_trim();
}

bool StudioSceneCache::has(const String &p_path) const {
	return _find(p_path) != -1;
}

void StudioSceneCache::erase(const String &p_path) {
	const int index = _find(p_path);
	if (index != -1) {
		entries.remove_at(index);
	}
}

void StudioSceneCache::clear() {
	entries.clear();
}

PackedStringArray StudioSceneCache::get_paths() const {
	PackedStringArray paths;
	for (const Entry &entry : entries) {
		paths.push_back(entry.path);
	}
	return paths;
}
