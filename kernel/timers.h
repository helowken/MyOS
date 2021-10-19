#ifndef _TIMERS_H
#define _TIMERS_H

#include "limits.h"
#include "sys/types.h"

struct Timer;

typedef void (*tmr_func_t)(struct Timer *tp);

typedef union {
	int ta_int;
	long ta_long;
	void *ta_ptr;
} tmr_arg_t;

typedef struct Timer {
	struct Timer *next;	/* Next in a timer chain */
	clock_t	expTime;	/* Expiration time */
	tmr_func_t func;		/* Function to call when expired */
	tmr_arg_t arg;		/* random argument */
} Timer;

/* Used when the timer is not active. */
#define TMR_NEVER		((clock_t) LONG_MAX)

/* These definitions can be used to set or get data from a timer variable. */
#define tmrArg(tp)		(&(tp)->arg)
#define tmrExpTime(tp)	(&(tp)->expTime)

/* The following generic timer management functions are available. They
 * can be used to operate on the lists of timers. Adding a timer to a list
 * automatically takes care of removing it.
 */
clock_t clearTimer(Timer **timers, Timer *tp, clock_t *newHead);
void expiredTimers(Timer **timers, clock_t now, clock_t *newHead);
clock_t setTimer(Timer **timers, Timer *tp, clock_t expTime, 
			tmr_func_t watchDog, clock_t *newHead);

#endif
