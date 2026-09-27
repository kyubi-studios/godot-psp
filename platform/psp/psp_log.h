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

// OOM izleme (malloc/realloc/calloc sarmalayıcıları): NULL dönen her tahsis "[PSP] FAIL OOM" loglar
// ve sayacı artırır. psp_oom_expected true iken (self-test) FAIL yerine "[PSP] OOM (expected)" yazar.
extern volatile bool psp_oom_expected;
extern volatile int psp_oom_count;
