// pspdev newlib'in bildirdiği ama libc'de uygulamadığı POSIX çağrıları.
// OS_Unix'in alt süreç yolları ve FileAccessUnix::resize bunlara bağlanır; PSP'de hepsi ENOSYS ile başarısız.

#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {

int dup2(int, int) {
	errno = ENOSYS;
	return -1;
}

pid_t waitpid(pid_t, int *, int) {
	errno = ENOSYS;
	return -1;
}

pid_t setsid(void) {
	errno = ENOSYS;
	return -1;
}

pid_t vfork(void) {
	errno = ENOSYS;
	return -1;
}

int sigaction(int, const struct sigaction *, struct sigaction *) {
	errno = ENOSYS;
	return -1;
}

int mkfifo(const char *, mode_t) {
	errno = ENOSYS;
	return -1;
}

int ftruncate(int, off_t) {
	errno = ENOSYS;
	return -1;
}

} // extern "C"
