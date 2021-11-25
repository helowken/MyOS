#include "timers.h"

#define NULL	((void *) 0)

void expiredTimers(Timer **timers, clock_t now, clock_t *newHead) {
/* Use the current time to check the timers queue list for expired timers.
 * Run the watchdog functions for all expired timers and deactivate them.
 * The caller is responsible for scheduling a new alarm if needed.
 */
	Timer *tp;
	
	while ((tp = *timers) != NULL && tp->expiredTime <= 0) {
		*timers = tp->next;
		tp->expiredTime = TIMER_NEVER;
		(*tp->func)(tp);
	}

	if (newHead) {
		if (*timers) 
		  *newHead = (*timers)->expiredTime;
		else
		  *newHead = 0;
	}
}
