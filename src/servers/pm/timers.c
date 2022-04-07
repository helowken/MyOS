#include "pm.h"

#include "timers.h"
#include "minix/syslib.h"
#include "minix/com.h"

static Timer *pmTimers = NULL;

void pmExpireTimers(clock_t now) {
	clock_t nextTime;

	/* Check for expired timers and possibly reschedule an alarm. */
	timersExpTimers(&pmTimers, now, &nextTime);
	if (nextTime > 0) {
		if (sysSetAlarm(nextTime, 1) != OK)
		  panic(__FILE__, "PM expire timer couldn't set alarm.", NO_NUM);
	}
}

void pmSetTimer(Timer *tp, clock_t ticks, TimerFunc watchdog, int arg) {
	int r;
	clock_t now, prevTime = 0, nextTime;
	
	if ((r = getUptime(&now)) != OK)
	  panic(__FILE__, "PM couldn't get uptime", NO_NUM);

	/* Set timer argument and add timer to the list. */
	timerArg(tp)->ta_int = arg;
	prevTime = timersSetTimer(&pmTimers, tp, now + ticks, watchdog, &nextTime);
	
	/* Reschedule our synchronous alarm if necessary. */
	if (! prevTime || prevTime > nextTime) {
		if (sysSetAlarm(nextTime, 1) != OK)
		  panic(__FILE__, "PM set timer couldn't set alarm.", NO_NUM);
	}
}

void pmCancelTimer(Timer *tp) {
	clock_t nextTime, prevTime;
	prevTime = timersClearTimer(&pmTimers, tp, &nextTime);

	/* If the earlist timer has been removed, we have to set the alarm to
	 * the next timer, or cancel the alarm altogether if the last timer has
	 * been cancelled (nextTime will be 0 then).
	 */
	if (prevTime < nextTime || ! nextTime) {
		if (sysSetAlarm(nextTime, 1) != OK)
		  panic(__FILE__, "PM expire timer couldn't set alarm.", NO_NUM);
	}
}
