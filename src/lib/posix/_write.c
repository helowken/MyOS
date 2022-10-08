#include "lib.h"
#include "unistd.h"

ssize_t write(int fd, const void *buf, size_t count) {
	Message msg;

	msg.m1_i1 = fd;
	msg.m1_i2 = count;
	msg.m1_p1 = (char *) buf;
	return syscall(FS, WRITE, &msg);
}
