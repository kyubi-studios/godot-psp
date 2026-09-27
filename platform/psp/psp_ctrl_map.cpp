#include "psp_ctrl_map.h"

#include <pspctrl.h>

struct PSPButtonMap {
	uint32_t psp;
	JoyButton godot;
};

static const PSPButtonMap BUTTONS[] = {
	{ PSP_CTRL_CROSS, JoyButton::A },
	{ PSP_CTRL_CIRCLE, JoyButton::B },
	{ PSP_CTRL_SQUARE, JoyButton::X },
	{ PSP_CTRL_TRIANGLE, JoyButton::Y },
	{ PSP_CTRL_LTRIGGER, JoyButton::LEFT_SHOULDER },
	{ PSP_CTRL_RTRIGGER, JoyButton::RIGHT_SHOULDER },
	{ PSP_CTRL_SELECT, JoyButton::BACK },
	{ PSP_CTRL_START, JoyButton::START },
	{ PSP_CTRL_UP, JoyButton::DPAD_UP },
	{ PSP_CTRL_DOWN, JoyButton::DPAD_DOWN },
	{ PSP_CTRL_LEFT, JoyButton::DPAD_LEFT },
	{ PSP_CTRL_RIGHT, JoyButton::DPAD_RIGHT },
};

int psp_ctrl_map(uint32_t p_prev, uint32_t p_cur, PSPJoyEvent *r_events, int p_max) {
	uint32_t changed = p_prev ^ p_cur;
	int n = 0;
	for (const PSPButtonMap &b : BUTTONS) {
		if (n >= p_max) {
			break;
		}
		if (changed & b.psp) {
			r_events[n].button = (int)b.godot;
			r_events[n].pressed = (p_cur & b.psp) != 0;
			n++;
		}
	}
	return n;
}

float psp_axis_map(uint8_t p_raw) {
	float v = (float(p_raw) - 127.5f) / 127.5f;
	if (v > 1.0f) {
		v = 1.0f;
	} else if (v < -1.0f) {
		v = -1.0f;
	}
	if (v > -0.15f && v < 0.15f) {
		return 0.0f;
	}
	return v;
}
