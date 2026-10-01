/**************************************************************************/
/*  studio_layout_pages.h                                                 */
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

#include "core/io/config_file.h"

// Layout pages are named editor layouts stored in `editor_layouts.cfg`, in the same
// format as the stock "Editor Layout" menu: a page is a section whose name contains
// no '/', and "<page>/..." sections hold additional data belonging to that page.
class StudioLayoutPages {
public:
	// Returns an empty string when the name is valid, otherwise an error message.
	static String validate_page_name(const String &p_name);

	static PackedStringArray list_pages(const Ref<ConfigFile> &p_config);
	static void erase_page(const Ref<ConfigFile> &p_config, const String &p_name);
	static Error copy_page(const Ref<ConfigFile> &p_config, const String &p_from, const String &p_to);
	static Error rename_page(const Ref<ConfigFile> &p_config, const String &p_from, const String &p_to);
};
