#include "fs.h"
#include "timers.h"
#include "minix/syslib.h"
#include "minix/com.h"

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
