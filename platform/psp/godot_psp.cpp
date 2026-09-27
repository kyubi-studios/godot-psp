#include "os_psp.h"
#include "psp_exit.h"
#include "psp_log.h"
#include "psp_paths.h"

#include "main/main.h"

#include <malloc.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PSP_MODULE_INFO("GodotPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_STACK_SIZE_KB(512);

int psp_selftest();

// <dir>/<name> dosyasındaki tamsayı; dosya yoksa -1.
static int psp_read_int_file(const char *p_dir, const char *p_name) {
	char path[300];
	snprintf(path, sizeof(path), "%s%s%s", p_dir, p_dir[strlen(p_dir) - 1] == '/' ? "" : "/", p_name);
	SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
	if (fd < 0) {
		return -1;
	}
	char num[16] = {};
	sceIoRead(fd, num, sizeof(num) - 1);
	sceIoClose(fd);
	return atoi(num);
}

int main(int argc, char *argv[]) {
	// newlib free() heap'i sbrk ile küçültmesin: arena monoton kalır ve gerçek peak (high-water) olur.
	mallopt(M_TRIM_THRESHOLD, 0x7fffffff);
	psp_exit_setup();
	psp_log("[PSP] main enter argv0=%s", argc > 0 ? argv[0] : "(none)");

	OS_PSP os;
	os.executable_path = String::utf8(argc > 0 ? argv[0] : "");

	char game_dir[256];
	psp_game_dir(argc > 0 ? argv[0] : nullptr, game_dir, sizeof(game_dir));
	psp_log("[PSP] game_dir=%s", game_dir);

	// Test modu: oyun klasöründe "psp_selftest" dosyası varsa (EBOOT argüman alamaz) ya da --psp-selftest verilmişse.
	bool selftest = false;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--psp-selftest") == 0) {
			selftest = true;
		}
	}
	{
		char marker[300];
		snprintf(marker, sizeof(marker), "%s%spsp_selftest", game_dir, game_dir[strlen(game_dir) - 1] == '/' ? "" : "/");
		SceIoStat st;
		if (sceIoGetstat(marker, &st) >= 0) {
			selftest = true;
		}
	}
	if (selftest) {
		int fails = psp_selftest();
		sceKernelExitGame();
		return fails;
	}

	// Test kancaları: oyun klasöründeki "psp_quit_after_frames" / "psp_screenshot_at_frame" dosyaları.
	os.quit_after_frames = psp_read_int_file(game_dir, "psp_quit_after_frames");
	os.screenshot_at_frame = psp_read_int_file(game_dir, "psp_screenshot_at_frame");
	if (os.quit_after_frames > 0) {
		psp_log("[PSP] quit_after_frames=%d", os.quit_after_frames);
	}

	// Oyun verisi: EBOOT'un yanında game.pck varsa --main-pack (export edilmiş oyun), yoksa klasör (--path).
	char pck_path[300];
	snprintf(pck_path, sizeof(pck_path), "%s%sgame.pck", game_dir, game_dir[strlen(game_dir) - 1] == '/' ? "" : "/");
	SceIoStat pck_stat;
	const bool use_pck = sceIoGetstat(pck_path, &pck_stat) >= 0;
	if (use_pck) {
		psp_log("[PSP] main_pack=%s", pck_path);
	}
	char *args[] = {
		(char *)(use_pck ? "--main-pack" : "--path"), use_pck ? pck_path : game_dir,
		(char *)"--display-driver", (char *)"psp",
		(char *)"--rendering-method", (char *)"dummy",
		(char *)"--audio-driver", (char *)"Dummy",
	};
	int nargs = sizeof(args) / sizeof(args[0]);

	Error err = Main::setup(argc > 0 ? argv[0] : "godot", nargs, args);
	uint32_t mem_used, mem_peak;
	psp_mem_stats(mem_used, mem_peak);
	psp_log("[PSP] setup err=%d mem=%u peak=%u", (int)err, (unsigned)mem_used, (unsigned)mem_peak);
	if (err != OK) {
		psp_log("[PSP] FAIL setup");
		sceKernelExitGame();
		return 1;
	}
	if (Main::start() == EXIT_SUCCESS) {
		psp_mem_stats(mem_used, mem_peak);
		psp_log("[PSP] start OK mem=%u peak=%u", (unsigned)mem_used, (unsigned)mem_peak);
		os.run();
	} else {
		psp_log("[PSP] FAIL start");
	}
	Main::cleanup();
	psp_log("[PSP] cleanup done");
	psp_exit_mark_main_done();
	sceKernelExitGame();
	return 0;
}
