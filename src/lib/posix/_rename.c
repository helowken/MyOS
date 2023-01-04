#include "lib.h"
#include "string.h"
#include "stdio.h"

int rename(const char *oldPath, const char *newPath) {
	Message msg;

	msg.m1_i1 = strlen(oldPath) + 1;
	msg.m1_i2 = strlen(newPath) + 1;
	msg.m1_p1 = (char *) oldPath;
	msg.m1_p2 = (char *) newPath;
	return syscall(FS, RENAME, &msg);
}
