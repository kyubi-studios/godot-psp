/**************************************************************************/
/*  studio_layout_pages.cpp                                               */
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


#include "studio_layout_pages.h"

static bool _is_page_section(const String &p_section, const String &p_page) {
	return p_section == p_page || p_section.begins_with(p_page + "/");
}

String StudioLayoutPages::validate_page_name(const String &p_name) {
	const String name = p_name.strip_edges();
	if (name.is_empty()) {
		return TTR("Page name can't be empty.");
	}
	if (name.contains_char('/') || name.contains_char('\\')) {
		return TTR("Page name contains invalid characters: \"/\" or \"\\\".");
	}
	return String();
}

PackedStringArray StudioLayoutPages::list_pages(const Ref<ConfigFile> &p_config) {
	PackedStringArray pages;
	if (p_config.is_null()) {
		return pages;
	}
	for (const String &section : p_config->get_sections()) {
		if (!section.contains_char('/')) {
			pages.push_back(section);
		}
	}
	return pages;
}

void StudioLayoutPages::erase_page(const Ref<ConfigFile> &p_config, const String &p_name) {
	ERR_FAIL_COND(p_config.is_null());
	for (const String &section : p_config->get_sections()) {
		if (_is_page_section(section, p_name)) {
			p_config->erase_section(section);
		}
	}
}

Error StudioLayoutPages::copy_page(const Ref<ConfigFile> &p_config, const String &p_from, const String &p_to) {
	ERR_FAIL_COND_V(p_config.is_null(), ERR_INVALID_PARAMETER);
	if (!validate_page_name(p_to).is_empty()) {
		return ERR_INVALID_PARAMETER;
	}
	if (!p_config->has_section(p_from)) {
		return ERR_DOES_NOT_EXIST;
	}
	if (p_config->has_section(p_to)) {
		return ERR_ALREADY_EXISTS;
	}

	for (const String &section : p_config->get_sections()) {
		if (!_is_page_section(section, p_from)) {
			continue;
		}
		const String target = p_to + section.substr(p_from.length());
		for (const String &key : p_config->get_section_keys(section)) {
			p_config->set_value(target, key, p_config->get_value(section, key));
		}
	}
	return OK;
}

Error StudioLayoutPages::rename_page(const Ref<ConfigFile> &p_config, const String &p_from, const String &p_to) {
	ERR_FAIL_COND_V(p_config.is_null(), ERR_INVALID_PARAMETER);
	if (p_from == p_to && p_config->has_section(p_from)) {
		return OK;
	}
	Error err = copy_page(p_config, p_from, p_to);
	if (err != OK) {
		return err;
	}
	erase_page(p_config, p_from);
	return OK;
}
