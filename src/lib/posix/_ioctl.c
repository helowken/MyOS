#include "lib.h"
#include "minix/com.h"
#include "sys/ioctl.h"

int ioctl(int fd, int request, void *data) {
	Message msg;

	msg.TTY_LINE = fd;
	msg.TTY_REQUEST = request;
	msg.ADDRESS = (char *) data;
	return syscall(FS, IOCTL, &msg);
}
