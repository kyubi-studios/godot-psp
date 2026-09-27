#pragma once

// PSP çıkış akışı (HOME → Çık):
// 1. psp_exit_setup() main()'in başında callback thread'ini kurar.
// 2. Sistem exit callback'i çağırınca bayrak set edilir; DisplayServerPSP bunu görür ve Godot'a
//    WINDOW_EVENT_CLOSE_REQUEST gönderir (oyun NOTIFICATION_WM_CLOSE_REQUEST ile kaydedebilir).
// 3. Watchdog: callback'ten sonra PSP_EXIT_WATCHDOG_US içinde main bitmezse (açılışta, takılmada ya da
//    auto_accept_quit=false) sceKernelExitGame() zorlanır.

constexpr unsigned int PSP_EXIT_WATCHDOG_US = 3 * 1000 * 1000;

void psp_exit_setup();
// Exit callback'ini sistemdeki gibi tetikler (test: psp_quit_after_frames).
void psp_request_exit();
bool psp_exit_requested();
// main() sceKernelExitGame'e ulaştığında çağrılır; watchdog'u susturur.
void psp_exit_mark_main_done();
