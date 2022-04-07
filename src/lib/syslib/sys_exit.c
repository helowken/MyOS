#include "syslib.h"

int sysExit(int pNum) {
	Message msg;

	msg.PR_PROC_NR = pNum;
	return taskCall(SYSTASK, SYS_EXIT, &msg);
}
