#include "display_server_psp.h"

#include "psp_ctrl_map.h"
#include "psp_exit.h"
#include "psp_log.h"

#include "core/input/input.h"
#include "servers/rendering/dummy/rasterizer_dummy.h"

#include <pspctrl.h>
#include <pspdisplay.h>

DisplayServerPSP::DisplayServerPSP() {
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	// DisplayServerHeadless kurucusu Input'u zaten kullanıyor; burada hazır.
	Input::get_singleton()->joy_connection_changed(0, true, "PSP");
	psp_log("[PSP] display_driver=psp");
}

Vector<String> DisplayServerPSP::get_rendering_drivers_func() {
	Vector<String> drivers;
	drivers.push_back("dummy");
	return drivers;
}

DisplayServer *DisplayServerPSP::create_func(const String &, DisplayServerEnums::WindowMode, DisplayServerEnums::VSyncMode, uint32_t, const Vector2i *, const Vector2i &, int, DisplayServerEnums::Context, int64_t, Error &r_error) {
	r_error = OK;
	RasterizerDummy::make_current();
	return memnew(DisplayServerPSP());
}

void DisplayServerPSP::register_psp_driver() {
	register_create_function("psp", create_func, get_rendering_drivers_func);
}

void DisplayServerPSP::process_events() {
	if (psp_exit_requested() && !close_request_sent) {
		close_request_sent = true;
		if (window_event_callback.is_valid()) {
			window_event_callback.call((int)DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
		}
		psp_log("[PSP] close request sent");
	}
	SceCtrlData pad;
	if (sceCtrlPeekBufferPositive(&pad, 1) <= 0) {
		return;
	}
	Input *input = Input::get_singleton();
	PSPJoyEvent events[12];
	int n = psp_ctrl_map(prev_buttons, pad.Buttons, events, 12);
	for (int i = 0; i < n; i++) {
		input->joy_button(0, (JoyButton)events[i].button, events[i].pressed);
	}
	prev_buttons = pad.Buttons;

	float ax = psp_axis_map(pad.Lx);
	float ay = psp_axis_map(pad.Ly);
	if (ax != prev_axis[0]) {
		input->joy_axis(0, JoyAxis::LEFT_X, ax);
		prev_axis[0] = ax;
	}
	if (ay != prev_axis[1]) {
		input->joy_axis(0, JoyAxis::LEFT_Y, ay);
		prev_axis[1] = ay;
	}
	input->flush_buffered_events();
}

void DisplayServerPSP::swap_buffers() {
	sceDisplayWaitVblankStart();
}
