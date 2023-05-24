#include <lib.h>
#include <string.h>
#include <unistd.h>

int link(const char *oldpath, const char *newpath) {
	Message msg;

	msg.m1_i1 = strlen(oldpath) + 1;
	msg.m1_i2 = strlen(newpath) + 1;
	msg.m1_p1 = (char *) oldpath;
	msg.m1_p2 = (char *) newpath;
	return syscall(FS, LINK, &msg);
}
