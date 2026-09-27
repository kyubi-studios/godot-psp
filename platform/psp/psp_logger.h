#pragma once

#include "core/io/logger.h"

// Godot print/hata çıktısını psp_log_raw'a (stdout + PPSSPP devctl) yönlendirir.
class PSPLogger : public Logger {
public:
	void logv(const char *p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;
};
