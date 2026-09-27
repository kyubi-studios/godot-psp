#include "os_psp.h"

#include "psp_log.h"

#include "core/config/project_settings.h"
#include "core/os/main_loop.h"
#include "main/main.h"
#include "servers/display/display_server.h"

#include <pspkernel.h>
#include <psprtc.h>

void OS_PSP::initialize() {
	OS_Unix::initialize_core();
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
		frame++;
		if ((frame % 60) == 0) {
			psp_log("[PSP] frame %d mem=%llu peak=%llu", frame,
					(unsigned long long)Memory::get_mem_usage(), (unsigned long long)Memory::get_mem_max_usage());
		}
		if (quit_after_frames > 0 && frame >= quit_after_frames) {
			break;
		}
	}
	main_loop->finalize();
	psp_log("[PSP] exit clean frames=%d", frame);
}

OS_PSP::OS_PSP() {
}
