#include <timers.h>

void timersExpTimers(Timer **timers, clock_t now, clock_t *newHead) {
/* Use the current time to check the timers queue list for expired timers.
 * Run the watchdog functions for all expired timers and deactivate them.
 * The caller is responsible for scheduling a new alarm if needed.
 */
	Timer *tp;
	
	while ((tp = *timers) != NULL && tp->tmr_exp_time <= now) {
		*timers = tp->tmr_next;
		tp->tmr_exp_time = TIMER_NEVER;
		(*tp->tmr_func)(tp);
	}

	if (newHead) {
		if (*timers) 
		  *newHead = (*timers)->tmr_exp_time;
		else
		  *newHead = 0;
	}
}
