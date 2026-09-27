#include "os_psp.h"

#include "display_server_psp.h"
#include "psp_log.h"
#include "psp_logger.h"

#include "core/config/project_settings.h"
#include "core/os/main_loop.h"
#include "main/main.h"
#include "servers/display/display_server.h"

#include <pspkernel.h>
#include <psprtc.h>

void OS_PSP::initialize() {
	OS_Unix::initialize_core();
	DisplayServerPSP::register_psp_driver();
}

void OS_PSP::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
}

void OS_PSP::delete_main_loop() {
	if (main_loop) {
		memdelete(main_loop);
	}
	main_loop = nullptr;
}

void OS_PSP::finalize() {
	delete_main_loop();
}

bool OS_PSP::_check_internal_feature_support(const String &p_feature) {
	return p_feature == "psp" || p_feature == "mobile";
}

Error OS_PSP::get_entropy(uint8_t *r_buffer, int p_bytes) {
	// Kriptografik değil; PSP'de güvenli kaynak kullanılmıyor.
	u64 tick = 0;
	sceRtcGetCurrentTick(&tick);
	for (int i = 0; i < p_bytes; i++) {
		tick = tick * 6364136223846793005ULL + 1442695040888963407ULL;
		r_buffer[i] = uint8_t(tick >> 56);
	}
	return OK;
}

void OS_PSP::run() {
	if (!main_loop) {
		return;
	}
	main_loop->initialize();
	int frame = 0;
	while (!quit_requested) {
		DisplayServer::get_singleton()->process_events();
		if (Main::iteration()) {
			break;
		}
		// Faz 1: dummy renderer swap_buffers çağırmaz; vblank beklemesini burada yapıyoruz.
		// Faz 2'de PSP GU renderer'ı end_frame'de sceGuSwapBuffers + vblank yapacak ve bu çağrı oraya taşınacak.
		DisplayServer::get_singleton()->swap_buffers();
		frame++;
		if ((frame % 60) == 0) {
			uint32_t used, peak;
			psp_mem_stats(used, peak);
			psp_log("[PSP] frame %d mem=%u peak=%u t=%llu", frame, (unsigned)used, (unsigned)peak,
					(unsigned long long)get_ticks_msec());
		}
		if (quit_after_frames > 0 && frame >= quit_after_frames) {
			break;
		}
	}
	main_loop->finalize();
	psp_log("[PSP] exit clean frames=%d", frame);
}

OS_PSP::OS_PSP() {
	// OS_Unix'in terminal logger'ı yerine PSP kanalı (PPSSPP headless çıktıyı yalnızca devctl'den gösterir).
	Vector<Logger *> loggers;
	loggers.push_back(memnew(PSPLogger));
	_set_logger(memnew(CompositeLogger(loggers)));
}
