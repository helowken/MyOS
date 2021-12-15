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
