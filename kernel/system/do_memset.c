#include "../system.h"

int doMemset(Message *msg) {
/* Handle sys_memset(). This writes a pattern into the specified memory. */
	unsigned long p;
	unsigned char c = msg->MEM_PATTERN;
	p = c | (c << 8) | (c << 16) | (c << 24);
	physMemset((phys_bytes) msg->MEM_PTR, p, (phys_bytes) msg->MEM_COUNT);
	return OK;
}
