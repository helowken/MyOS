#include <lib.h>
#include <unistd.h>

int pipe(int fds[2]) {
	Message msg;

	if (syscall(FS, PIPE, &msg) < 0)
	  return -1;

	fds[0] = msg.m1_i1;
	fds[1] = msg.m1_i2;
	return 0;
}
