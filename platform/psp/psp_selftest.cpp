#include "psp_ctrl_map.h"
#include "psp_log.h"
#include "psp_paths.h"

#include <stdlib.h>
#include <string.h>

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

	// psp_game_dir: argv[0] → oyun klasörü (Review I5).
	char dir[64];
	psp_game_dir("umd0:/EBOOT.PBP", dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, "umd0:/") == 0, "game_dir umd0 root");
	psp_game_dir("ms0:/PSP/GAME/GodotDemo/EBOOT.PBP", dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, "ms0:/PSP/GAME/GodotDemo") == 0, "game_dir ms0 folder");
	psp_game_dir("", dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, ".") == 0, "game_dir empty argv0");
	psp_game_dir(nullptr, dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, ".") == 0, "game_dir null argv0");
	psp_game_dir("EBOOT.PBP", dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, ".") == 0, "game_dir no slash");

	// Peak, örnekler arasında serbest bırakılan tahsisleri de görmeli (Review I3).
	// GCC malloc/free çiftlerini silip sonucu katlayabilir; volatile işaretçiyle gerçek çağrıyı zorla.
	void *(*volatile do_malloc)(size_t) = malloc;
	void (*volatile do_free)(void *) = free;
	uint32_t used0, peak0, used1, peak1;
	psp_mem_stats(used0, peak0);
	const size_t spike = 2 * 1024 * 1024;
	void *p = do_malloc(spike);
	if (p) {
		memset(p, 0xAB, spike);
	}
	do_free(p);
	psp_mem_stats(used1, peak1);
	PSP_CHECK(p != nullptr && peak1 >= peak0 + spike, "peak sees freed spike");

	// Bellek tükenmesi ayırt edici bir log satırı üretmeli (Review I3).
	psp_oom_expected = true;
	void *huge = do_malloc(200 * 1024 * 1024);
	psp_oom_expected = false;
	PSP_CHECK(huge == nullptr, "huge malloc fails");
	PSP_CHECK(psp_oom_count == 1, "oom counted");
	do_free(huge);

	psp_log("[PSP] selftest done fails=%d", fails);
	return fails;
}
