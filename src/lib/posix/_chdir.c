#include <lib.h>
#include <unistd.h>

int chdir(const char *path) {
	Message msg;

	_loadName(path, &msg);
	return syscall(FS, CHDIR, &msg);
}

int fchdir(int fd) {
	Message msg;

	msg.m1_i1 = fd;
	return syscall(FS, FCHDIR, &msg);
}
