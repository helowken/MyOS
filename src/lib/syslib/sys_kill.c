#include "syslib.h"

int sysKill(int pNum, int sigNum) {
	Message msg;

	msg.SIG_PROC = pNum;
	msg.SIG_NUMBER = sigNum;
	return taskCall(SYSTASK, SYS_KILL, &msg);
}
