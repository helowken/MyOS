#include "timers.h"

clock_t timersSetTimer(Timer **timers, Timer *tp, clock_t expTime, 
			timerFunc watchdog, clock_t *newHead) {
/* Activate a timer to run function 'watchdog' at time 'expTime'. If the timer is
 * already in use it is first removed from the timers queue. Then, it is put
 * in the list of active timers with the first to expire in front.
 * The caller responsible for scheduling a new alarm for the timer if needed.
 */
	Timer **atp;
	clock_t oldHead = 0;

	if (*timers)
	  oldHead = (*timers)->expiredTime;
	
	timersClearTimer(timers, tp, NULL);
	tp->expiredTime = expTime;
	tp->func = watchdog;

	/* Add the timer to the active timers. The next timer due is in front. */
	for (atp = timers; *atp != NULL; atp = &(*atp)->next) {
		if (expTime < (*atp)->expiredTime)
		  break;
	}
	tp->next = *atp;
	*atp = tp;
	if (newHead)
	  *newHead = (*timers)->expiredTime;

	return oldHead;
}
