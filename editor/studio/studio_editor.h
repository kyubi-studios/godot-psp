/**************************************************************************/
/*  studio_editor.h                                                       */
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

#include "scene/main/node.h"

class Control;
class EditorBottomPanel;
class EditorTitleBar;

// Root of the Studio editor extensions. Owned by EditorNode; everything Studio adds
// to the editor UI is created and wired from here so upstream files only need one hook.
class StudioEditor : public Node {
	GDCLASS(StudioEditor, Node);

	static inline StudioEditor *singleton = nullptr;

	EditorTitleBar *title_bar = nullptr;
	Control *title_right_container = nullptr;
	EditorBottomPanel *bottom_panel = nullptr;

public:
	static StudioEditor *get_singleton() { return singleton; }

	void setup(EditorTitleBar *p_title_bar, Control *p_title_right_container, EditorBottomPanel *p_bottom_panel);

	StudioEditor();
	~StudioEditor();
};
