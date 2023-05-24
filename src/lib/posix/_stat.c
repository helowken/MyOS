#include <lib.h>
#include <sys/stat.h>
#include <string.h>

int stat(const char *path, struct stat *buf) {
	Message msg;

	msg.m1_i1 = strlen(path) + 1;
	msg.m1_p1 = (char *) path;
	msg.m1_p2 = (char *) buf;
	return syscall(FS, STAT, &msg);
}
