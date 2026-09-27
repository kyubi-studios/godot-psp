#pragma once

// PSP: dinamik kütüphane yükleme yok. OS_Unix derlenebilsin diye her çağrı başarısız döner.

#define RTLD_LAZY 0x0001
#define RTLD_NOW 0x0002
#define RTLD_GLOBAL 0x0100
#define RTLD_LOCAL 0
#define RTLD_NEXT ((void *)-1)
#define RTLD_DEFAULT ((void *)0)

#ifdef __cplusplus
extern "C" {
#endif

static inline void *dlopen(const char *, int) { return 0; }
static inline void *dlsym(void *, const char *) { return 0; }
static inline int dlclose(void *) { return -1; }
static inline char *dlerror(void) { return (char *)"dynamic libraries are not supported on PSP"; }

#ifdef __cplusplus
}
#endif
