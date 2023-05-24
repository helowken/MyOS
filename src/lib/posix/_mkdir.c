#include <lib.h>
#include <sys/stat.h>
#include <string.h>

int mkdir(const char *path, mode_t mode) {
	Message msg;

	msg.m1_i1 = strlen(path) + 1;
	msg.m1_i2 = mode;
	msg.m1_p1 = (char *) path;
	return syscall(FS, MKDIR, &msg);
}
