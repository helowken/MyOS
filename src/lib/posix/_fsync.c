#include "lib.h"
#include "unistd.h"

int fsync(int fd) {
	Message msg;

	msg.m1_i1 = fd;

	return syscall(FS, FSYNC, &msg);
}
