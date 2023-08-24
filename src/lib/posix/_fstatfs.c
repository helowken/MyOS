#include <lib.h>
#include <sys/stat.h>
#include <sys/statfs.h>

int fstatfs(int fd, struct statfs *st) {
	Message msg;

	msg.m1_i1 = fd;
	msg.m1_p1 = (char *) st;
	return syscall(FS, FSTATFS, &msg);
}
