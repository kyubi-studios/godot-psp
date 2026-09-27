#include "psp_paths.h"

#include <string.h>

void psp_game_dir(const char *p_argv0, char *r_out, size_t p_size) {
	if (p_size < 3) {
		if (p_size > 0) {
			r_out[0] = 0;
		}
		return;
	}
	const char *slash = p_argv0 ? strrchr(p_argv0, '/') : nullptr;
	if (!slash) {
		// argv[0] boş/NULL ya da klasör içermiyor: libcglue cwd'yi zaten argv[0]'dan kurar.
		strcpy(r_out, ".");
		return;
	}
	size_t len = (size_t)(slash - p_argv0);
	if (len + 2 > p_size) {
		len = p_size - 2;
	}
	memcpy(r_out, p_argv0, len);
	r_out[len] = 0;
	// Cihaz kökü (ör. PPSSPP'nin "umd0:/EBOOT.PBP") → "umd0:/"; chdir("umd0:") başarısız olur.
	if (len > 0 && r_out[len - 1] == ':') {
		r_out[len] = '/';
		r_out[len + 1] = 0;
	}
}
