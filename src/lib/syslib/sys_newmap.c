#include "syslib.h"

int sysNewMap(int pNum, MemMap *memMap) {
/* A process has been assigned a new memory map. Tell the kernel. */
	Message msg;

	msg.PR_PROC_NR = pNum;
	msg.PR_MEM_PTR = (char *) memMap;
	return taskCall(SYSTASK, SYS_NEWMAP, &msg);
}
