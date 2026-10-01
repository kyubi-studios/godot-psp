/**************************************************************************/
/*  test_studio_editor.cpp                                                */
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

TEST_FORCE_LINK(test_studio_editor)

#ifdef TOOLS_ENABLED

#include "core/input/input_event.h"
#include "editor/settings/editor_settings.h"
#include "editor/studio/studio_editor.h"

namespace TestStudioEditor {

TEST_CASE("[Studio] StudioEditor singleton lifetime") {
	CHECK(StudioEditor::get_singleton() == nullptr);

	StudioEditor *studio = memnew(StudioEditor);
	CHECK(StudioEditor::get_singleton() == studio);

	memdelete(studio);
	CHECK(StudioEditor::get_singleton() == nullptr);
}

TEST_CASE("[Editor][Studio] Studio settings and page shortcuts are registered") {
	StudioEditor::register_settings();

	CHECK(bool(EDITOR_GET("interface/studio/pages/auto_save_on_switch")));
	CHECK(EditorSettings::get_singleton()->has_setting("interface/studio/bottom_drawer/auto_hide"));
	CHECK_FALSE(bool(EDITOR_GET("interface/studio/bottom_drawer/auto_hide")));

	Ref<InputEventKey> key;
	key.instantiate();
	key->set_keycode(Key::KEY_3);
	key->set_ctrl_pressed(true);
	key->set_alt_pressed(true);
	key->set_pressed(true);
	CHECK(ED_IS_SHORTCUT("studio/page_3", key));
	CHECK_FALSE(ED_IS_SHORTCUT("studio/page_2", key));

	key->set_keycode(Key::KEY_9);
	CHECK(ED_IS_SHORTCUT("studio/page_9", key));
}

TEST_CASE("[Editor][Studio] Dock region toggle shortcuts are registered") {
	StudioEditor::register_settings();

	Ref<InputEventKey> key;
	key.instantiate();
	key->set_ctrl_pressed(true);
	key->set_alt_pressed(true);
	key->set_pressed(true);

	key->set_keycode(Key::COMMA);
	CHECK(ED_IS_SHORTCUT("studio/toggle_left_docks", key));
	CHECK_FALSE(ED_IS_SHORTCUT("studio/toggle_right_docks", key));

	key->set_keycode(Key::PERIOD);
	CHECK(ED_IS_SHORTCUT("studio/toggle_right_docks", key));
}

} // namespace TestStudioEditor

#endif // TOOLS_ENABLED
