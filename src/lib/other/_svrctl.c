#include "lib.h"
#include "stdio.h"
#include "sys/svrctl.h"

int svrctl(int request, void *data) {
	Message msg;

	msg.m2_i1 = request;
	msg.m2_p1 = data;

	switch ((request >> 8) & 0xFF) {
		case 'M':
		case 'S':
			/* MM handles calls for itself and the kernel. */
			return syscall(MM, SVRCTL, &msg);
		case 'F':
		case 'I':
			/* FS handles calls for itself and inet. */
			return syscall(FS, SVRCTL, &msg);
		default:
			errno = EINVAL;
			return -1;
	}
}
