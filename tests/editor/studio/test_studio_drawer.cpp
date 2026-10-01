/**************************************************************************/
/*  test_studio_drawer.cpp                                                */
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

TEST_FORCE_LINK(test_studio_drawer)

#ifdef TOOLS_ENABLED

#include "editor/studio/studio_drawer.h"

namespace TestStudioDrawer {

// Arguments: enabled, pinned, panel_open, focus_inside_panel, focus_in_popup.

TEST_CASE("[Studio] Drawer hides when focus leaves an open, unpinned panel") {
	CHECK(StudioDrawer::should_auto_hide(true, false, true, false, false));
}

TEST_CASE("[Studio] Drawer never hides when the feature is disabled") {
	CHECK_FALSE(StudioDrawer::should_auto_hide(false, false, true, false, false));
}

TEST_CASE("[Studio] Drawer stays open while pinned") {
	CHECK_FALSE(StudioDrawer::should_auto_hide(true, true, true, false, false));
}

TEST_CASE("[Studio] Drawer does nothing when the panel is already closed") {
	CHECK_FALSE(StudioDrawer::should_auto_hide(true, false, false, false, false));
}

TEST_CASE("[Studio] Drawer stays open while focus is inside the panel") {
	CHECK_FALSE(StudioDrawer::should_auto_hide(true, false, true, true, false));
}

TEST_CASE("[Studio] Drawer stays open when focus moves into a popup or dialog") {
	CHECK_FALSE(StudioDrawer::should_auto_hide(true, false, true, false, true));
}

} // namespace TestStudioDrawer

#endif // TOOLS_ENABLED
