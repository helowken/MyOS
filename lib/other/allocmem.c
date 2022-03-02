#include "lib.h"
#include "unistd.h"

int allocMem(phys_bytes size, phys_bytes *base) {
	Message msg;

	msg.m4_l1 = size;
	if (syscall(MM, ALLOCMEM, &msg) < 0)
	  return -1;
	*base = msg.m4_l2;
	return 0;
}
