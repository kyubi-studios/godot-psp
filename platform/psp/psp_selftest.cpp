#include "psp_ctrl_map.h"
#include "psp_log.h"

#include <pspctrl.h>

static int fails = 0;

#define PSP_CHECK(m_cond, m_msg)                   \
	if (!(m_cond)) {                               \
		psp_log("[PSP] FAIL selftest %s", m_msg); \
		fails++;                                   \
	}

int psp_selftest() {
	PSPJoyEvent ev[16];

	int n = psp_ctrl_map(0, PSP_CTRL_CROSS, ev, 16);
	PSP_CHECK(n == 1 && ev[0].button == (int)JoyButton::A && ev[0].pressed, "cross press");

	n = psp_ctrl_map(PSP_CTRL_CROSS, 0, ev, 16);
	PSP_CHECK(n == 1 && ev[0].button == (int)JoyButton::A && !ev[0].pressed, "cross release");

	n = psp_ctrl_map(0, PSP_CTRL_UP | PSP_CTRL_RTRIGGER | PSP_CTRL_START, ev, 16);
	PSP_CHECK(n == 3, "three buttons at once");

	n = psp_ctrl_map(PSP_CTRL_UP, PSP_CTRL_UP, ev, 16);
	PSP_CHECK(n == 0, "held button no event");

	n = psp_ctrl_map(0, 0xFFFFFFFF, ev, 2);
	PSP_CHECK(n == 2, "event buffer limit");

	PSP_CHECK(psp_axis_map(128) == 0.0f, "axis center dead zone");
	PSP_CHECK(psp_axis_map(0) <= -0.99f, "axis min");
	PSP_CHECK(psp_axis_map(255) >= 0.99f, "axis max");
	PSP_CHECK(psp_axis_map(140) == 0.0f, "axis small offset dead zone");

	psp_log("[PSP] selftest done fails=%d", fails);
	return fails;
}
