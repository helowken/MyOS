#ifndef _TIMERS_H
#define _TIMERS_H

#include "limits.h"
#include "sys/types.h"
#include "stddef.h"

struct Timer;

typedef void (*TimerFunc)(struct Timer *tp);

typedef union {
	int ta_int;
	long ta_long;
	void *ta_ptr;
} TimerArg;

typedef struct Timer {
	struct Timer *tmr_next;		/* Next in a timer chain */
	clock_t	tmr_exp_time;		/* Expiration time */
	TimerFunc tmr_func;			/* Function to call when expired */
	TimerArg tmr_arg;			/* random argument */
} Timer;

/* Used when the timer is not active. */
#define TIMER_NEVER		((clock_t) LONG_MAX)

/* These definitions can be used to set or get data from a timer variable. */
#define timerArg(tp)	(&(tp)->tmr_arg)
#define timerExpTime(tp)	(&(tp)->tmr_exp_time)

/* Timers should be initialized once before they are being used. */
#define timerInit(tp)	{ (tp)->tmr_exp_time = TIMER_NEVER;	\
							(tp)->tmr_next = NULL; }

/* The following generic timer management functions are available. They
 * can be used to operate on the lists of timers. Adding a timer to a list
 * automatically takes care of removing it.
 */
clock_t timersClearTimer(Timer **timers, Timer *tp, clock_t *newHead);
void timersExpTimers(Timer **timers, clock_t now, clock_t *newHead);
clock_t timersSetTimer(Timer **timers, Timer *tp, clock_t expTime, 
			TimerFunc watchdog, clock_t *newHead);

#endif
