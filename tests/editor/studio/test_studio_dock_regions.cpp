/**************************************************************************/
/*  test_studio_dock_regions.cpp                                          */
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

TEST_FORCE_LINK(test_studio_dock_regions)

#ifdef TOOLS_ENABLED

#include "editor/docks/editor_dock_manager.h"

namespace TestStudioDockRegions {

TEST_CASE("[Studio] Dock slots map to their screen region") {
	using DM = EditorDockManager;
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_LEFT_UL) == DM::DOCK_REGION_LEFT);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_LEFT_BL) == DM::DOCK_REGION_LEFT);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_LEFT_UR) == DM::DOCK_REGION_LEFT);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_LEFT_BR) == DM::DOCK_REGION_LEFT);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_RIGHT_UL) == DM::DOCK_REGION_RIGHT);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_RIGHT_BL) == DM::DOCK_REGION_RIGHT);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_RIGHT_UR) == DM::DOCK_REGION_RIGHT);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_RIGHT_BR) == DM::DOCK_REGION_RIGHT);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_BOTTOM) == DM::DOCK_REGION_BOTTOM);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_BOTTOM_L) == DM::DOCK_REGION_BOTTOM);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_BOTTOM_R) == DM::DOCK_REGION_BOTTOM);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_NONE) == DM::DOCK_REGION_NONE);
	CHECK(DM::get_slot_region(EditorDock::DOCK_SLOT_MAX) == DM::DOCK_REGION_NONE);
}

TEST_CASE("[Studio] Dock columns map to their screen region") {
	using DM = EditorDockManager;
	// The editor creates four dock columns: left-left, left-right, right-left, right-right.
	CHECK(DM::get_vsplit_region(0, 4) == DM::DOCK_REGION_LEFT);
	CHECK(DM::get_vsplit_region(1, 4) == DM::DOCK_REGION_LEFT);
	CHECK(DM::get_vsplit_region(2, 4) == DM::DOCK_REGION_RIGHT);
	CHECK(DM::get_vsplit_region(3, 4) == DM::DOCK_REGION_RIGHT);
	CHECK(DM::get_vsplit_region(4, 4) == DM::DOCK_REGION_NONE);
	CHECK(DM::get_vsplit_region(-1, 4) == DM::DOCK_REGION_NONE);
}

} // namespace TestStudioDockRegions

#endif // TOOLS_ENABLED
