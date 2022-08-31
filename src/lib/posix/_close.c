#include "lib.h"
#include "unistd.h"

int close(int fd) {
	Message msg;

	msg.m1_i1 = fd;
	return syscall(FS, CLOSE, &msg);
}
