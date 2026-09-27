#pragma once

#include <cstddef>

// argv[0]'ın dizini (ör. "ms0:/PSP/GAME/GodotDemo/EBOOT.PBP" → "ms0:/PSP/GAME/GodotDemo").
// Cihaz kökü "umd0:/" olarak döner; argv[0] boş, NULL ya da '/' içermiyorsa ".".
void psp_game_dir(const char *p_argv0, char *r_out, size_t p_size);
