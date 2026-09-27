#include "os_psp.h"
#include "psp_log.h"

#include "main/main.h"

#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PSP_MODULE_INFO("GodotPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_STACK_SIZE_KB(512);

static OS_PSP *g_os = nullptr;

int psp_selftest();

static int psp_exit_callback(int, int, void *) {
	if (g_os) {
		g_os->quit_requested = true;
	}
	return 0;
}

static int psp_callback_thread(SceSize, void *) {
	int cbid = sceKernelCreateCallback("exit_cb", psp_exit_callback, nullptr);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

static void psp_setup_callbacks() {
	int thid = sceKernelCreateThread("cb_thread", psp_callback_thread, 0x11, 0x1000, THREAD_ATTR_USER, nullptr);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, nullptr);
	}
}

// argv[0]'ın dizini (ör. "ms0:/PSP/GAME/GodotDemo/EBOOT.PBP" → "ms0:/PSP/GAME/GodotDemo").
static void psp_game_dir(const char *p_argv0, char *r_out, size_t p_size) {
	strncpy(r_out, p_argv0 ? p_argv0 : "", p_size - 1);
	r_out[p_size - 1] = 0;
	char *slash = strrchr(r_out, '/');
	if (slash) {
		*slash = 0;
	}
	// Cihaz kökü (ör. PPSSPP'nin "umd0:/EBOOT.PBP") → "umd0:/"; chdir("umd0:") başarısız olur.
	size_t len = strlen(r_out);
	if (len > 0 && r_out[len - 1] == ':' && len + 1 < p_size) {
		r_out[len] = '/';
		r_out[len + 1] = 0;
	}
}

int main(int argc, char *argv[]) {
	psp_setup_callbacks();
	psp_log("[PSP] main enter argv0=%s", argc > 0 ? argv[0] : "(none)");

	OS_PSP os;
	g_os = &os;
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

	// Test: oyun klasöründe "psp_quit_after_frames" dosyası varsa içindeki kare sayısından sonra çık.
	{
		char path[300];
		snprintf(path, sizeof(path), "%s%spsp_quit_after_frames", game_dir, game_dir[strlen(game_dir) - 1] == '/' ? "" : "/");
		SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
		if (fd >= 0) {
			char num[16] = {};
			sceIoRead(fd, num, sizeof(num) - 1);
			sceIoClose(fd);
			os.quit_after_frames = atoi(num);
			psp_log("[PSP] quit_after_frames=%d", os.quit_after_frames);
		}
	}

	char *args[] = {
		(char *)"--path", game_dir,
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
	sceKernelExitGame();
	return 0;
}
