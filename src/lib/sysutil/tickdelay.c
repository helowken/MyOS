#include "sysutil.h"
#include <timers.h>

int tickDelay(clock_t ticks) {
/* This function uses the synchronous alarm to delay for a while. This works
 * even if a previous synchronous alarm was scheduled, because the remaining
 * tick of the previous alarm are returned so that it can be rescheduled.
 * Note however that a long tick_delay (longer than the remaining time of the
 * previous) alarm will also delay the previous alarm.
 */
	Message msg;
	clock_t timeLeft;
	int s;

	if (ticks <= 0)
	  return EINVAL;

	msg.ALARM_EXP_TIME = ticks;		/* Request message after ticks */
	msg.ALARM_ABS_TIME = 0;			/* Ticks are relative to now */

	if ((s = taskCall(SYSTASK, SYS_SETALARM, &msg)) != OK)
	  return s;

	receive(CLOCK, &msg);			/* Await synchronous alarm */

	/* Check if we must reschedule the current alarm. */
	timeLeft = msg.ALARM_TIME_LEFT;
	if (timeLeft > 0 && timeLeft != TIMER_NEVER) {
		msg.ALARM_EXP_TIME = timeLeft - ticks;
		if (msg.ALARM_EXP_TIME <= 0)
		  msg.ALARM_EXP_TIME = 1;
		s = taskCall(SYSTASK, SYS_SETALARM, &msg);	
	}

	return s;
}
