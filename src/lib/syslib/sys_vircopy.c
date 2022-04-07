#include "syslib.h"

int sysVirCopy(int srcProc, int srcSeg, vir_bytes srcVir,
	int dstProc, int dstSeg, vir_bytes dstVir, phys_bytes bytes) {
/* Transfer a block of data. The source and destination can each either be a
 * process number of SELF (to indicate own process number). Virtual addresses
 * are offsets within LOCAL_SEG (text, stack, data), REMOTE_SEG, or BIOS_SEG.
 */
	Message msg;
	
	if (bytes == 0)
	  return OK;
	msg.CP_SRC_PROC_NR = srcProc;
	msg.CP_SRC_SPACE = srcSeg;
	msg.CP_SRC_ADDR = (long) srcVir;
	msg.CP_DST_PROC_NR = dstProc;
	msg.CP_DST_SPACE = dstSeg;
	msg.CP_DST_ADDR = (long) dstVir;
	msg.CP_NR_BYTES = (long) bytes;
	return taskCall(SYSTASK, SYS_VIRCOPY, &msg);
}
