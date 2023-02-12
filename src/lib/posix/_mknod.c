#include "lib.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"

int mknod(const char *path, mode_t mode, dev_t dev) {
	Message msg;

	msg.m1_i1 = strlen(path) + 1;
	msg.m1_i2 = mode;
	msg.m1_i3 = dev;
	msg.m1_p1 = (char *) path;
	return syscall(FS, MKNOD, &msg);
}
