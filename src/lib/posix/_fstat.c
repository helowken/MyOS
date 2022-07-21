#include "lib.h"
#include "sys/stat.h"

int fstat(int fd, struct stat *buf) {
	Message msg;

	msg.m1_i1 = fd;
	msg.m1_p1 = (char *) buf;
	return syscall(FS, FSTAT, &msg);
}
