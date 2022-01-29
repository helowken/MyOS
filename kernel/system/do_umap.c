#include "../system.h"

int doUMap(register Message *msg) {
/* Map virtual address to physical, for non-kernel processes. */
	int segType = msg->CP_SRC_SPACE & SEGMENT_TYPE;
	int segIdx = msg->CP_SRC_SPACE & SEGMENT_INDEX;
	vir_bytes offset = msg->CP_SRC_ADDR;
	int count = msg->CP_NR_BYTES;
	int pNum = (int) msg->CP_SRC_PROC_NR;
	phys_bytes physAddr;

	/* Verify process number. */
	if (pNum == SELF) 
	  pNum = msg->m_source;
	if (! isOkProcNum(pNum))
	  return EINVAL;

	/* See which mapping should be made. */
	switch (segType) {
		case LOCAL_SEG:
			physAddr = umapLocal(procAddr(pNum), segIdx, offset, count);
			break;
		case REMOTE_SEG:
			physAddr = umapRemote(procAddr(pNum), segIdx, offset, count);
			break;
		case BIOS_SEG:
			physAddr = umapBios(procAddr(pNum), offset, count);
			break;
		default:
			return EINVAL;
	}

	msg->CP_DST_ADDR = physAddr;
	return physAddr == 0 ? EFAULT : OK;
}
