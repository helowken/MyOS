#include "syslib.h"

int sysUMap(int pNum, int seg, vir_bytes virAddr, vir_bytes bytes, phys_bytes *physAddr) {
	Message msg;
	int result;

	msg.CP_SRC_PROC_NR = pNum;
	msg.CP_SRC_SPACE = seg;
	msg.CP_SRC_ADDR = virAddr;
	msg.CP_NR_BYTES = bytes;

	result = taskCall(SYSTASK, SYS_UMAP, &msg);
	*physAddr = msg.CP_DST_ADDR;
	return result;
}
