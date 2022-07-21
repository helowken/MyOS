#include "timers.h"
 
clock_t timersClearTimer(Timer **timers, Timer *tp, clock_t *nextTime) {
/* Deactivate a timer and remove it from the timers queue. */
	Timer **atp;
	clock_t prevTime;

	if (*timers) 
	  prevTime = (*timers)->tmr_exp_time;
	else
	  prevTime = 0;

	tp->tmr_exp_time = TIMER_NEVER;

	for (atp = timers; *atp != NULL; atp = &(*atp)->tmr_next) {
		if (*atp == tp) {
			*atp = tp->tmr_next;
			break;
		}
	}

	if (nextTime) {
		if (*timers)
		  *nextTime = (*timers)->tmr_exp_time;
		else
		  *nextTime = 0;
	}

	return prevTime;
}
