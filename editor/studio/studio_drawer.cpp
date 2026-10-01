/**************************************************************************/
/*  studio_drawer.cpp                                                     */
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


#include "studio_drawer.h"

#include "core/object/callable_mp.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"

bool StudioDrawer::should_auto_hide(bool p_enabled, bool p_pinned, bool p_panel_open, bool p_focus_inside_panel, bool p_focus_in_popup) {
	return p_enabled && !p_pinned && p_panel_open && !p_focus_inside_panel && !p_focus_in_popup;
}

void StudioDrawer::setup(EditorBottomPanel *p_bottom_panel) {
	bottom_panel = p_bottom_panel;
}

void StudioDrawer::_focus_changed(Control *p_control) {
	if (!bottom_panel || !p_control) {
		return;
	}
	const bool enabled = EDITOR_GET("interface/studio/bottom_drawer/auto_hide");
	const bool panel_open = bottom_panel->get_current_tab() != -1;
	const bool focus_inside = bottom_panel->is_ancestor_of(p_control);
	// Focus in a popup, dialog or floating dock window must not close the drawer.
	const bool focus_in_popup = p_control->get_window() != bottom_panel->get_window();

	if (should_auto_hide(enabled, bottom_panel->is_locked(), panel_open, focus_inside, focus_in_popup)) {
		bottom_panel->hide_bottom_panel();
	}
}

void StudioDrawer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			get_viewport()->connect("gui_focus_changed", callable_mp(this, &StudioDrawer::_focus_changed));
		} break;
		case NOTIFICATION_EXIT_TREE: {
			get_viewport()->disconnect("gui_focus_changed", callable_mp(this, &StudioDrawer::_focus_changed));
		} break;
	}
}
