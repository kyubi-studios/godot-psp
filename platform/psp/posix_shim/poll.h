#pragma once

// PSP: poll() yok. Yalnızca OS_Unix'in alt süreç (pipe) yolları kullanır; PSP'de alt süreç yok.

#include <errno.h>

#define POLLIN 0x0001
#define POLLPRI 0x0002
#define POLLOUT 0x0004
#define POLLERR 0x0008
#define POLLHUP 0x0010
#define POLLNVAL 0x0020

typedef unsigned int nfds_t;

struct pollfd {
	int fd;
	short events;
	short revents;
};

#ifdef __cplusplus
extern "C" {
#endif

static inline int poll(struct pollfd *, nfds_t, int) {
	errno = ENOSYS;
	return -1;
}

#ifdef __cplusplus
}
#endif
