#include "psp_log.h"

#include <malloc.h>
#include <pspiofilemgr.h>
#include <stdarg.h>
#include <stdio.h>

// PPSSPP "emulator:" devctl komutları (Core/HLE/sceIo.cpp). Gerçek PSP'de sessizce başarısız olur.
static constexpr unsigned int EMU_DEVCTL_SEND_OUTPUT = 2;
static constexpr unsigned int EMU_DEVCTL_EMIT_SCREENSHOT = 0x20;

void psp_log_raw(const char *p_text, int p_len) {
	if (p_len <= 0) {
		return;
	}
	fwrite(p_text, 1, p_len, stdout);
	sceIoDevctl("emulator:", EMU_DEVCTL_SEND_OUTPUT, (void *)p_text, p_len, nullptr, 0);
}

void psp_log(const char *p_format, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, p_format);
	int n = vsnprintf(buf, sizeof(buf) - 1, p_format, ap);
	va_end(ap);
	if (n < 0) {
		return;
	}
	if (n > (int)sizeof(buf) - 2) {
		n = (int)sizeof(buf) - 2;
	}
	buf[n++] = '\n';
	buf[n] = 0;
	psp_log_raw(buf, n);
}

void psp_screenshot() {
	sceIoDevctl("emulator:", EMU_DEVCTL_EMIT_SCREENSHOT, nullptr, 0, nullptr, 0);
}

void psp_mem_stats(uint32_t &r_used, uint32_t &r_peak) {
	// uordblks: ayrılmış bayt. arena: sbrk ile alınan toplam heap (parçalanma dahil) — gerçek ayak izi.
	// newlib usmblks'i doldurmaz; peak'i arena'nın gördüğümüz en yüksek değeri olarak tutarız.
	static uint32_t peak = 0;
	struct mallinfo mi = mallinfo();
	if ((uint32_t)mi.arena > peak) {
		peak = (uint32_t)mi.arena;
	}
	r_used = (uint32_t)mi.uordblks;
	r_peak = peak;
}

volatile bool psp_oom_expected = false;
volatile int psp_oom_count = 0;
