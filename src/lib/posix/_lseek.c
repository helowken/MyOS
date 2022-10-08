#include "lib.h"
#include "unistd.h"

off_t lseek(int fd, off_t offset, int whence) {
	Message msg;

	msg.m2_i1 = fd;
	msg.m2_l1 = offset;
	msg.m2_i2 = whence;
	if (syscall(FS, LSEEK, &msg) < 0)
	  return (off_t) -1;
	return (off_t) msg.m2_l1;
}
