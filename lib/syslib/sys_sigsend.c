#include "syslib.h"

int sysSigSend(int pNum, SigMsg *sigCtx) {
	Message msg;
	int result;

	msg.SIG_PROC = pNum;
	msg.SIG_CTXT_PTR = (char *) sigCtx;
	result = taskCall(SYSTASK, SYS_SIGSEND, &msg);
	return result;
}
