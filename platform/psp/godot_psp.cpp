#include "os_psp.h"
#include "psp_log.h"

#include "main/main.h"

#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("GodotPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_STACK_SIZE_KB(512);

static OS_PSP *g_os = nullptr;

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
	fputs(buf, stdout);
	sceIoDevctl("emulator:", 2, buf, n, nullptr, 0);
}

void psp_screenshot() {
	sceIoDevctl("emulator:", 0x20, nullptr, 0, nullptr, 0);
}

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
}

int main(int argc, char *argv[]) {
	psp_setup_callbacks();
	psp_log("[PSP] main enter argv0=%s", argc > 0 ? argv[0] : "(none)");

	OS_PSP os;
	g_os = &os;

	char game_dir[256];
	psp_game_dir(argc > 0 ? argv[0] : nullptr, game_dir, sizeof(game_dir));
	psp_log("[PSP] game_dir=%s", game_dir);

	char *args[] = {
		(char *)"--path", game_dir,
		(char *)"--display-driver", (char *)"headless",
		(char *)"--rendering-method", (char *)"dummy",
		(char *)"--audio-driver", (char *)"Dummy",
	};
	int nargs = sizeof(args) / sizeof(args[0]);

	Error err = Main::setup(argc > 0 ? argv[0] : "godot", nargs, args);
	psp_log("[PSP] setup err=%d mem=%llu", (int)err, (unsigned long long)Memory::get_mem_usage());
	if (err != OK) {
		psp_log("[PSP] FAIL setup");
		sceKernelExitGame();
		return 1;
	}
	if (Main::start() == EXIT_SUCCESS) {
		psp_log("[PSP] start OK mem=%llu", (unsigned long long)Memory::get_mem_usage());
		os.run();
	} else {
		psp_log("[PSP] FAIL start");
	}
	Main::cleanup();
	psp_log("[PSP] cleanup done");
	sceKernelExitGame();
	return 0;
}
