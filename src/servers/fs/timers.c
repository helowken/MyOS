#include "fs.h"
#include <timers.h>
#include <minix/syslib.h>
#include <minix/com.h>

static Timer *fsTimers = NULL;

void fsInitTimer(Timer *tp) {
	timerInit(tp);
}

void fsExpireTimers(clock_t now) {
	clock_t newHead;

	timersExpTimers(&fsTimers, now, &newHead);
	if (newHead > 0) {
		if (sysSetAlarm(newHead, 1) != OK)
		  panic(__FILE__, "FS expire timer couldn't set "
					  "synchronous alarm.", NO_NUM);
	}
}

void fsCancelTimer(Timer *tp) {
	clock_t newHead, oldHead;
	oldHead = timersClearTimer(&fsTimers, tp, &newHead);

	/* If the earliest timer has been removed, we have to set
	 * the synalarm to the next timer, or cancel the synalarm
	 * altogether if the last time has been cancelled (newHead
	 * will be 0 then).
	 */
	if (oldHead < newHead || ! newHead) {
		if (sysSetAlarm(newHead, 1) != OK)
		  panic(__FILE__, 
			"FS expire timer couldn't set synchronous alarm.",
			NO_NUM);
	}
}

void fsSetTimer(Timer *tp, int delta, TimerFunc watchDog, int arg) {
	int r;
	clock_t now, oldHead = 0, newHead;

	if ((r = getUptime(&now)) != OK)
	  panic(__FILE__, 
			"FS couldn't get uptime from system task.", NO_NUM);

	timerArg(tp)->ta_int = arg;

	oldHead = timersSetTimer(&fsTimers, tp, now + delta, watchDog, &newHead);

	/* Reschedule our synchronous alarm if necessary */
	if (! oldHead || oldHead > newHead) {
		if (sysSetAlarm(newHead, 1) != OK)
		  panic(__FILE__, 
				"FS set timer couldn't set synchronous alarm.", NO_NUM);
	}
}
