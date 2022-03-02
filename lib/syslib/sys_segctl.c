#include "syslib.h"

int sysSegCtl(
	int *index,		/* Return index of remote segment */
	u16_t *segSel,	/* Return segment selector here */
	vir_bytes *off,		/* Return offset in segment here */
	phys_bytes physAddr,	/* Physical address to convert */
	vir_bytes size	/* Size of segment */
) {
	Message msg;
	int s;

	msg.SEG_PHYS_ADDR = physAddr;
	msg.SEG_SIZE = size;
	s = taskCall(SYSTASK, SYS_SEGCTL, &msg);
	*index = (int) msg.SEG_INDEX;
	*segSel = (u16_t) msg.SEG_SELECT;
	*off = (vir_bytes) msg.SEG_OFFSET;
	return s;
}
