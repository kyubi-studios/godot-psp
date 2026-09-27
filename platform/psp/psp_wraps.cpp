// Bağlayıcı sarmalayıcıları (detect.py: -Wl,--wrap=...).
// - malloc/realloc/calloc: NULL dönüşünü "[PSP] FAIL OOM" olarak loglar (bellek tükenmesi görünür olsun).
// - getcwd: libcglue cihaz kökünü "umd0:" olarak döndürür ama chdir("umd0:") başarısız olur; "umd0:/" yapar.

#include "psp_log.h"

#include <stddef.h>
#include <string.h>

extern "C" {

void *__real_malloc(size_t p_size);
void *__real_realloc(void *p_ptr, size_t p_size);
void *__real_calloc(size_t p_count, size_t p_size);
char *__real_getcwd(char *p_buf, size_t p_size);

static void psp_report_oom(const char *p_func, size_t p_size) {
	psp_oom_count = psp_oom_count + 1;
	if (psp_oom_expected) {
		psp_log("[PSP] OOM (expected) %s size=%u", p_func, (unsigned)p_size);
	} else {
		uint32_t used, peak;
		psp_mem_stats(used, peak);
		psp_log("[PSP] FAIL OOM %s size=%u used=%u peak=%u", p_func, (unsigned)p_size, (unsigned)used, (unsigned)peak);
	}
}

void *__wrap_malloc(size_t p_size) {
	void *p = __real_malloc(p_size);
	if (!p && p_size) {
		psp_report_oom("malloc", p_size);
	}
	return p;
}

void *__wrap_realloc(void *p_ptr, size_t p_size) {
	void *p = __real_realloc(p_ptr, p_size);
	if (!p && p_size) {
		psp_report_oom("realloc", p_size);
	}
	return p;
}

void *__wrap_calloc(size_t p_count, size_t p_size) {
	void *p = __real_calloc(p_count, p_size);
	if (!p && p_count && p_size) {
		psp_report_oom("calloc", p_count * p_size);
	}
	return p;
}

char *__wrap_getcwd(char *p_buf, size_t p_size) {
	char *r = __real_getcwd(p_buf, p_size);
	if (r && r == p_buf) {
		size_t len = strlen(r);
		if (len > 0 && r[len - 1] == ':' && len + 1 < p_size) {
			r[len] = '/';
			r[len + 1] = 0;
		}
	}
	return r;
}

} // extern "C"
