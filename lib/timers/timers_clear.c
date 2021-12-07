#include "timers.h"

#define NULL	((void *) 0)

clock_t timersClearTimer(Timer **timers, Timer *tp, clock_t *nextTime) {
/* Deactivate a timer and remove it from the timers queue. */
	Timer **atp;
	clock_t prevTime;

	if (*timers) 
	  prevTime = (*timers)->expiredTime;
	else
	  prevTime = 0;

	tp->expiredTime = TIMER_NEVER;

	for (atp = timers; *atp != NULL; atp = &(*atp)->next) {
		if (*atp == tp) {
			*atp = tp->next;
			break;
		}
	}

	if (nextTime) {
		if (*timers)
		  *nextTime = (*timers)->expiredTime;
		else
		  *nextTime = 0;
	}

	return prevTime;
}
