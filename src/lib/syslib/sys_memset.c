#include "syslib.h"

int sysMemset(unsigned long pattern, phys_bytes base, phys_bytes bytes) {
/* Zero a block of data. */
	Message msg;

	if (bytes == 0L)
	  return OK;

	msg.MEM_PTR = (char *) base;
	msg.MEM_COUNT = bytes;
	msg.MEM_PATTERN = pattern;

	return taskCall(SYSTASK, SYS_MEMSET, &msg);
}
