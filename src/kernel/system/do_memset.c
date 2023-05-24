#include "../system.h"

int doMemSet(Message *msg) {
/* Handle sys_memset(). This writes a pattern into the specified memory. */
	unsigned long p;
	unsigned char c = msg->MEM_PATTERN;
	int count = (phys_bytes) msg->MEM_COUNT;
	if (count < 0)
	  return EINVAL;
	if (count == 0)
	  return OK;
	p = c | (c << 8) | (c << 16) | (c << 24);
	physMemset((phys_bytes) msg->MEM_PTR, p, count);
	return OK;
}
