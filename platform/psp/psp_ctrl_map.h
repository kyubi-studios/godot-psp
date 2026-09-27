#pragma once

#include "core/input/input_enums.h"

#include <cstdint>

struct PSPJoyEvent {
	int button;
	bool pressed;
};

int psp_ctrl_map(uint32_t p_prev, uint32_t p_cur, PSPJoyEvent *r_events, int p_max);
float psp_axis_map(uint8_t p_raw);
