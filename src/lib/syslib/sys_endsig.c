#include "syslib.h"

int sysEndKernelSig(int pNum) {
	Message msg;
	int result;

	msg.SIG_PROC = pNum;
	result = taskCall(SYSTEM, SYS_ENDKSIG, &msg);
	return result;
}
