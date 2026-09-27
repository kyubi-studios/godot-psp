#pragma once

#include <cstdint>

// PSP log: stdout + PPSSPP "emulator:" SEND_OUTPUT devctl. Satır başına "[PSP] " yazmak çağıranın işidir.
void psp_log(const char *p_format, ...) __attribute__((format(printf, 1, 2)));
// Ham metin (satır sonu eklemez); Godot logger'ı kullanır.
void psp_log_raw(const char *p_text, int p_len);
void psp_screenshot();

// newlib mallinfo: o an ayrılmış bayt (uordblks) ve heap'in (arena) gördüğümüz en yüksek boyutu.
// Godot'un Memory sayaçları release derlemesinde 0 döndürür, bu yüzden heap'i doğrudan ölçüyoruz.
void psp_mem_stats(uint32_t &r_used, uint32_t &r_peak);
