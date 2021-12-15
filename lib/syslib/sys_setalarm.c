#include "syslib.h"

int sysSetAlarm(clock_t expTime, int absTime) {
/* Ask the SYSTEM schedule a synchronous alarm for the caller. The process
 * number can be SELF if the caller doesn't know its process number.
 */
	Message msg;
	msg.ALARM_EXP_TIME = expTime;	/* The expiration time */
	msg.ALARM_ABS_TIME = absTime;	/* Time is absolute? */
	return taskCall(SYSTASK, SYS_SETALARM, &msg);
}
