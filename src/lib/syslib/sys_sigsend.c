#include "syslib.h"

int sysSigSend(int pNum, SigMsg *sigMsg) {
	Message msg;
	int result;

	msg.SIG_PROC = pNum;
	msg.SIG_CTXT_PTR = (char *) sigMsg;
	result = taskCall(SYSTASK, SYS_SIGSEND, &msg);
	return result;
}
