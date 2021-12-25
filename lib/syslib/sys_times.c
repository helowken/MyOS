#include "syslib.h"

int sysTimes(int pNum, clock_t t[5]) {
/* Fetch the accounting info for a proc. */
	Message msg;
	int r;

	msg.T_PROC_NR = pNum;
	r = taskCall(SYSTASK, SYS_TIMES, &msg);
	t[0] = msg.T_USER_TIME;
	t[1] = msg.T_SYSTEM_TIME;
	t[2] = msg.T_CHILD_UTIME;
	t[3] = msg.T_CHILD_STIME;
	t[4] = msg.T_BOOT_TICKS;
	return r;
}
