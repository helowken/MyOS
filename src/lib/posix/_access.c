#include "lib.h"
#include "unistd.h"

int access(const char *path, int mode) {
	Message msg;

	msg.m3_i2 = mode;
	_loadName(path, &msg);
	return syscall(FS, ACCESS, &msg);
}
