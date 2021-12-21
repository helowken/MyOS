#include "syslib.h"

int sysSigReturn(int pNum, SigMsg *sigMsg) {
	Message msg;
	int result;

	msg.SIG_PROC = pNum;
	msg.SIG_CTXT_PTR = (char *) sigMsg;
	result = taskCall(SYSTASK, SYS_SIGRETURN, &msg);
	return result;
}
