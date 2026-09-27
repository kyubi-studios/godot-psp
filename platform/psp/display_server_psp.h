#pragma once

#include "servers/display/display_server_headless.h"

class DisplayServerPSP : public DisplayServerHeadless {
	GDSOFTCLASS(DisplayServerPSP, DisplayServerHeadless);

	uint32_t prev_buttons = 0;
	float prev_axis[2] = { 0.0f, 0.0f };
	Callable window_event_callback;
	bool close_request_sent = false;

	static DisplayServer *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();

public:
	static void register_psp_driver();

	String get_name() const override { return "psp"; }
	void process_events() override;
	void swap_buffers() override;
	void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override { window_event_callback = p_callable; }
	Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return Size2i(480, 272); }
	Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return Size2i(480, 272); }
	// Headless tabanı "çizilemez/minimize" bildirir; PSP'de tek tam ekran pencere her zaman çizilir.
	bool can_any_window_draw() const override { return true; }
	bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return true; }
	int get_screen_count() const override { return 1; }
	DisplayServerEnums::WindowMode window_get_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN; }
	float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return 60.0f; }

	DisplayServerPSP();
};
