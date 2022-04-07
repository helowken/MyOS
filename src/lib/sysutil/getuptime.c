#include "sysutil.h"

clock_t getUptime(clock_t *ticks) {
	Message msg;
	int s;

	msg.T_PROC_NR = NONE;		/* Ignore process times */
	s = taskCall(SYSTASK, SYS_TIMES, &msg);
	*ticks = (clock_t) msg.T_BOOT_TICKS;
	return s;
}
