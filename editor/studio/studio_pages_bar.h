/**************************************************************************/
/*  studio_pages_bar.h                                                    */
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
#include "scene/gui/box_container.h"

class Button;
class ButtonGroup;
class ConfirmationDialog;
class InputEvent;
class Label;
class LineEdit;
class PopupMenu;

// Title bar strip with one button per layout page (see StudioLayoutPages).
// Clicking a page saves the current dock layout into the active page (optional)
// and loads the clicked one; open scenes and tabs are left untouched.
class StudioPagesBar : public HBoxContainer {
	GDCLASS(StudioPagesBar, HBoxContainer);

	enum ContextOption {
		CONTEXT_SAVE_HERE,
		CONTEXT_RENAME,
		CONTEXT_DELETE,
	};

	enum NameDialogMode {
		NAME_DIALOG_NEW,
		NAME_DIALOG_RENAME,
	};

	String config_path;
	String current_page;
	String context_page;
	PackedStringArray pages;

	Ref<ButtonGroup> button_group;
	LocalVector<Button *> page_buttons;
	Button *add_button = nullptr;
	PopupMenu *context_menu = nullptr;

	ConfirmationDialog *name_dialog = nullptr;
	LineEdit *name_edit = nullptr;
	Label *name_error = nullptr;
	NameDialogMode name_dialog_mode = NAME_DIALOG_NEW;

	Ref<ConfigFile> _load_config() const;
	Error _save_config(const Ref<ConfigFile> &p_config);
	void _layouts_changed();

	void _page_button_pressed(const String &p_page);
	void _page_button_gui_input(const Ref<InputEvent> &p_event, const String &p_page);
	void _context_option(int p_option);

	void _add_button_pressed();
	void _popup_name_dialog(NameDialogMode p_mode, const String &p_initial);
	void _name_text_changed(const String &p_text);
	void _name_confirmed();

protected:
	static void _bind_methods();

public:
	void set_config_path(const String &p_path);
	String get_config_path() const;

	void refresh();

	void set_current_page(const String &p_page);
	String get_current_page() const;

	void switch_to_page(const String &p_page);
	void switch_to_index(int p_index);
	Error save_current_layout_as(const String &p_page);
	Error rename_page(const String &p_from, const String &p_to);
	void delete_page(const String &p_page);

	StudioPagesBar();
};
