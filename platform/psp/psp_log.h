#pragma once

// PSP log: stdout + PPSSPP "emulator:" SEND_OUTPUT devctl. Satır başına "[PSP] " yazmak çağıranın işidir.
void psp_log(const char *p_format, ...) __attribute__((format(printf, 1, 2)));
void psp_screenshot();
