#include <lib.h>
#include <sys/stat.h>

int chmod(const char *path, mode_t mode) {
	Message msg;

	msg.m3_i2 = mode;
	_loadName(path, &msg);
	return syscall(FS, CHMOD, &msg);
}
