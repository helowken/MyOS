#ifndef _TIMERS_H
#define _TIMERS_H

#include "limits.h"
#include "sys/types.h"

struct Timer;

typedef void (*timerFunc)(struct Timer *tp);

typedef union {
	int ta_int;
	long ta_long;
	void *ta_ptr;
} TimerArg;

typedef struct Timer {
	struct Timer *next;		/* Next in a timer chain */
	clock_t	expiredTime;	/* Expiration time */
	timerFunc func;			/* Function to call when expired */
	TimerArg arg;			/* random argument */
} Timer;

/* Used when the timer is not active. */
#define TIMER_NEVER		((clock_t) LONG_MAX)

/* These definitions can be used to set or get data from a timer variable. */
#define timerArg(tp)	(&(tp)->arg)
#define timerExpTime(tp)	(&(tp)->expredTime)

/* Timers should be initialized once before they are being used. */
#define timerInit(tp)	{ (tp)->expiredTime = TIMER_NEVER;	\
							(tp)->next = NULL; }

/* The following generic timer management functions are available. They
 * can be used to operate on the lists of timers. Adding a timer to a list
 * automatically takes care of removing it.
 */
clock_t clearTimer(Timer **timers, Timer *tp, clock_t *newHead);
void expiredTimers(Timer **timers, clock_t now, clock_t *newHead);
clock_t setTimer(Timer **timers, Timer *tp, clock_t expiredTime, 
			timerFunc watchDog, clock_t *newHead);

#endif
