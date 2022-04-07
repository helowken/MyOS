#include "../system.h"
#include "../protect.h"

int doSegCtl(register Message *msg) {
/*Return a segment selector and offset that can be used to reach a physical
 * address, for use by a driver doing memory I/O in the A0000 - DFFFF range.
 */
	u16_t selector;
	vir_bytes offset;
	int i, index;
	register Proc *rp;
	Priv *sp;
	phys_bytes physAddr = (phys_bytes) msg->SEG_PHYS_ADDR;
	vir_bytes size = (vir_bytes) msg->SEG_SIZE;

	rp = procAddr(msg->m_source);
	sp = priv(rp);
	index = -1;
	for (i = 0; i < NR_REMOTE_SEGS; ++i) {
		if (!sp->s_far_mem[i].inUse) {
			index = i;
			sp->s_far_mem[i].inUse = true;
			sp->s_far_mem[i].physAddr = physAddr;
			sp->s_far_mem[i].len = size;
			break;
		}
	}
	if (index < 0)
	  return ENOSPC;

	/* Check if the segment size can be recorded in bytes, that is, check
	 * if descriptor's limit field can delimited the allowed memory region
	 * precisely. This works up to 1MB. If the size is larger, 4K pages
	 * instead of bytes are used.
	 */
	if (size < BYTE_GRAN_MAX) {
		initDataSeg(&rp->p_ldt[EXTRA_LDT_INDEX + i], physAddr, size,
					USER_PRIVILEGE);
		offset = 0;
	} else {
		initDataSeg(&rp->p_ldt[EXTRA_LDT_INDEX + i], physAddr & ~0xFFFF, 0,
					USER_PRIVILEGE);
		offset = physAddr & 0xFFFF;
	}
	selector = ((EXTRA_LDT_INDEX + i) * DESC_SIZE) | T1 | USER_PRIVILEGE;

	/* Request successfully done. Now return the result. */
	msg->SEG_INDEX = index | REMOTE_SEG;
	msg->SEG_SELECT = selector;
	msg->SEG_OFFSET = offset;
	return OK;
}
