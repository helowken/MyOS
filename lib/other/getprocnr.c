#include "lib.h"
#include "unistd.h"

int getProcNum() {
	Message msg;

	msg.m1_i1 = -1;		/* Get own process number */
	msg.m1_i2 = 0;		/* Get own process number */
	if (syscall(MM, GETPROCNR, &msg) < 0)
	  return -1;
	return msg.m1_i1;
}
