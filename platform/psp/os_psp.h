#pragma once

#include "drivers/unix/os_unix.h"

class OS_PSP : public OS_Unix {
	MainLoop *main_loop = nullptr;

protected:
	void initialize() override;
	void set_main_loop(MainLoop *p_main_loop) override;
	void delete_main_loop() override;
	void finalize() override;
	bool _check_internal_feature_support(const String &p_feature) override;

public:
	int quit_after_frames = -1; // psp_quit_after_frames dosyası (test için)
	int screenshot_at_frame = -1; // psp_screenshot_at_frame dosyası (test için): o kareden sonra ekran görüntüsü

	String get_name() const override { return "PSP"; }
	String get_distribution_name() const override { return "PSP"; }
	String get_version() const override { return "6.61"; }
	MainLoop *get_main_loop() const override { return main_loop; }
	Vector<String> get_video_adapter_driver_info() const override { return Vector<String>(); }
	void initialize_joypads() override {}
	String get_executable_path() const override { return executable_path; }
	String executable_path; // argv[0], ör. "ms0:/PSP/GAME/GodotDemo/EBOOT.PBP" veya PPSSPP'de "umd0:/EBOOT.PBP"
	Error get_entropy(uint8_t *r_buffer, int p_bytes) override;
	void run();

	OS_PSP();
};
