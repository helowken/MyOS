#include <lib.h>
#include <fcntl.h>

int creat(const char *path, mode_t mode) {
	Message msg;

	msg.m3_i2 = mode;
	_loadName(path, &msg);
	return syscall(FS, CREAT, &msg);
}
