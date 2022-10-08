#include "syslib.h"

int sysExec(int pNum, char *newSp, char *progName, vir_bytes initPc) {
/* A process has exec'd. Tell the kernel. */

	Message msg;

	msg.PR_PROC_NR = pNum;
	msg.PR_STACK_PTR = newSp;
	msg.PR_NAME_PTR = progName;
	msg.PR_IP_PTR = (char *) initPc;

	return taskCall(SYSTASK, SYS_EXEC, &msg);
}
