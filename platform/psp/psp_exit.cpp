#include "psp_exit.h"

#include "psp_log.h"

#include <pspkernel.h>

static volatile bool exit_requested = false;
static volatile bool main_done = false;
static volatile SceUID exit_cbid = -1;

static int psp_exit_callback(int, int, void *) {
	psp_log("[PSP] exit callback");
	exit_requested = true;
	sceKernelDelayThread(PSP_EXIT_WATCHDOG_US);
	if (!main_done) {
		psp_log("[PSP] watchdog exit");
		sceKernelExitGame();
	}
	return 0;
}

static int psp_callback_thread(SceSize, void *) {
	exit_cbid = sceKernelCreateCallback("exit_cb", psp_exit_callback, nullptr);
	sceKernelRegisterExitCallback(exit_cbid);
	sceKernelSleepThreadCB();
	return 0;
}

void psp_exit_setup() {
	SceUID thid = sceKernelCreateThread("cb_thread", psp_callback_thread, 0x11, 0x1000, THREAD_ATTR_USER, nullptr);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, nullptr);
	}
}

void psp_request_exit() {
	if (exit_cbid >= 0) {
		sceKernelNotifyCallback(exit_cbid, 0);
	} else {
		exit_requested = true;
	}
}

bool psp_exit_requested() {
	return exit_requested;
}

void psp_exit_mark_main_done() {
	main_done = true;
}
