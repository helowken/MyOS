#include "syslib.h"

int sysFork(int parent, int child) {
	Message msg;

	msg.PR_PPROC_NR = parent;
	msg.PR_PROC_NR = child;
	return taskCall(SYSTASK, SYS_FORK, &msg);
}
