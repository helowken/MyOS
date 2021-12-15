#include "syslib.h"

int sysGetKernelSig(int *pNum, sigset_t *sigMap) {
	Message msg;
	int result;

	result = taskCall(SYSTASK, SYS_GETKSIG, &msg);
	*pNum = msg.SIG_PROC;
	*sigMap = (sigset_t) msg.SIG_MAP;
	return result;
}
