#pragma once
#include <pspiofilemgr.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// PPSSPP "emulator:" devctl komutları (Core/HLE/sceIo.cpp). Gerçek PSP'de sessizce başarısız olur.
#define PSP_EMU_DEVCTL_SEND_OUTPUT 2
#define PSP_EMU_DEVCTL_EMIT_SCREENSHOT 0x20

static inline void psp_test_log(const char *fmt, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	if (n < 0) {
		return;
	}
	if (n > (int)sizeof(buf) - 2) {
		n = (int)sizeof(buf) - 2;
	}
	buf[n++] = '\n';
	buf[n] = 0;
	fputs(buf, stdout);
	sceIoDevctl("emulator:", PSP_EMU_DEVCTL_SEND_OUTPUT, buf, n, NULL, 0);
}

static inline void psp_test_screenshot(void) {
	sceIoDevctl("emulator:", PSP_EMU_DEVCTL_EMIT_SCREENSHOT, NULL, 0, NULL, 0);
}
