#include "psp_logger.h"

#include "psp_log.h"

#include <stdio.h>

void PSPLogger::logv(const char *p_format, va_list p_list, bool p_err) {
	if (!should_log(p_err)) {
		return;
	}
	char buf[512];
	int n = vsnprintf(buf, sizeof(buf), p_format, p_list);
	if (n < 0) {
		return;
	}
	if (n >= (int)sizeof(buf)) {
		n = sizeof(buf) - 1;
	}
	psp_log_raw(buf, n);
}
