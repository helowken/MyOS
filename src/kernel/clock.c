/*
 * ====== PIT (Programmable Interval Timer) ======
 *
 * I/O port	  Usage
 * 0x40		  Channel 0 data port (read/write)
 * 0x41		  Channel 1 data port (read/write) (obsolete)
 * 0x42		  Channel 2 data port (read/write)
 * 0x43		  Mode/COmmand register (write only, a read is ignored)
 * 
 * Each 8 bit data port is the same, and is used to set the counter's 
 * 16 bit reload value or read the channel's 16 bit current count.
 *
 * The Mode/COmmand register at I/O address 0x43 contains the following:
 * Bits		  Usage
 * 6..7		  Select channel:
 *				0 0 = Channel 0
 *				0 1 = Channel 1
 *				1 0 = Channel 2
 *				1 1 = ead-back command (8254 only)
 * 4..5		  Access mode;
 *				0 0 = Latch count value command
 *				0 1 = Access mode: low byte only
 *				1 0 = Access mode: high byte only
 *				1 1 = Access mode: low byte + high byte
 * 1..3		  Operating mode:
 *				0 0 0 = Mode 0 (interrupt on terminal count)
 *				0 0 1 = Mode 1 (hardware re-triggerable one-shot)
 *				0 1 0 = Mode 2 (rate generator)
 *				0 1 1 = Mode 3 (square wave generator)
 *				1 0 0 = Mode 4 (software triggered strobe)
 *				1 0 1 = Mode 5 (hardware triggered strobe)
 *				1 1 0 = Mode 2 (rate generator, same as 010b)
 *				1 1 1 = Mode 3 (square wave generator, same as 011b)
 * 0		  BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
 *
 *
 */


#include "kernel.h"
#include "proc.h"
#include <stddef.h>
#include <minix/com.h>

/* Clock parameters. */
#define LATCH_COUNT		0x00	/* Channel=0, Access=Latch count value command,
								   Operating mode=interrupt on terminal count, 
								   Binary mode=16-bit binary. */
#define SQUARE_WAVE		0x36	/* Channel=0, Access=low byte + high byte, 
								   Operating mode=square wave generator, 
								   Binary mode=16-bit binary. */
#define TIMER_COUNT	((unsigned) (TIMER_FREQ / HZ))	/* Initial value for counter */
#define TIMER_FREQ	1193182L	/* Clock frequency for timer in PC and AT */

#define CLOCK_ACK_BIT	0x80	/* PS/2 clock interrupt acknowledge bit */

/* When a timer expires its watchdog function is run by the CLOCK task. */
static Timer *clockTimers;		/* Queue of CLOCK timers */
static clock_t nextTimeout;		/* Realtime that next timer expires */

/* The time is incremented by the interrupt handler on each clock tick. */
static clock_t realTime;		/* Real time clock */
static IrqHook clockHook;		/* Interrupt handler hook */


static int clockHandler(IrqHook *hook) {
	register unsigned ticks;

	/* Acknowledge the PS/2 clock interrupt. */
	if (machine.ps_mca)
	  outb(PORT_B, inb(PORT_B) | CLOCK_ACK_BIT);

	/* Get number of ticks and update realtime. */
	ticks = lostTicks + 1;
	lostTicks = 0;
	realTime += ticks;

	/* Update user and system accounting times. Charge the current process for
	 * user time. If the current process is not billable, that is, if a non-user
	 * process is running, charge the billable process for system time as well.
	 * Thus the unbillable process' user time is the billable user's system time.
	 */
	currProc->p_user_time += ticks;
	if (priv(currProc)->s_flags & PREEMPTIBLE) {
		currProc->p_ticks_left -= ticks;
	}
	if (! (priv(currProc)->s_flags & BILLABLE)) {
		billProc->p_sys_time += ticks;
		billProc->p_ticks_left -= ticks;
	}
	
	/* Check if doClickTick() must be called. Done for alarms and scheduling.
	 * Some processes, such as the kernel tasks, cannot be preempted.
	 */
	if (nextTimeout <= realTime || currProc->p_ticks_left <= 0) {
		prevProc = currProc;			/* Store running process */
		lockNotify(HARDWARE, CLOCK);	/* Send notifications */
	}
	return 1;		/* reenable interrupts */
}

static void initClock() {
	/* Initialize the CLOCK's interrupt hook. */
	clockHook.pNum = CLOCK;

	/* Initialize channel 0 of the 8253A timer to, e.g., 60 HZ. */
	outb(TIMER_MODE, SQUARE_WAVE);		/* Set timer to run continuously */
	outb(TIMER0, (u8_t) TIMER_COUNT);			/* Load timer low byte */
	outb(TIMER0, (u8_t) (TIMER_COUNT >> 8));		/* Load timer high byte */
	putIrqHandler(&clockHook, CLOCK_IRQ, clockHandler);	/* Register handler */
	enableIrq(&clockHook);
}

/* Despite its name, this routine is not called on every clock tick. It
 * is called on those clock ticks when a lot of work needs to be done.
 */
static int doClockTick() {
	/* A process used up a full quantum. The interrupt handler stored this
	 * process in 'prevProc'. First make sure that the process is not on the
	 * scheduling queues. Then announce the process ready again. Since it has
	 * no more time left, it gets a new quantum and is inserted at the right
	 * place in the queues. As a side-effect a new process will be scheduled.
	 */
	if (prevProc->p_ticks_left <= 0 && priv(prevProc)->s_flags & PREEMPTIBLE) {
		lockDequeue(prevProc);		/* Take it off the queue */
		lockEnqueue(prevProc);		/* and reinsert it again. */
	}

	/* Check if a clock timer expired and run its watchdog function. */
	if (nextTimeout <= realTime) {
		timersExpTimers(&clockTimers, realTime, NULL);
		nextTimeout = clockTimers == NULL ? TIMER_NEVER : clockTimers->tmr_exp_time;
	}

	return EDONTREPLY;
}

/* Main program of clock task. if the call is not HARD_INT it is an error. */
void clockTask() {
	Message m;			/* Message buffer for both input and output */

	initClock();		/* Initialize clock task */

	/* Main loop of the clock task. Get work, process it. Never reply. */
	while (true) {
		/* Go get a message */
		receive(ANY, &m);

		/* Handle the request. Only clock ticks are expected. */
		switch (m.m_type) {
			case HARD_INT:
				doClockTick(&m);	/* Handle clock tick */
				break;
			default:	/* Illegal request type */
				kprintf("CLOCK: illegal request %d from %d.\n", m.m_type, m.m_source);
		}
	}
}

/* Get and return the current clock uptime in ticks. */
clock_t getUptime() {
	return realTime;
}

void setTimer(Timer *tp, clock_t expTime, TimerFunc watchdog) {
/* Insert the new timer in the active timers list. Always update the
 * next timeout time by setting it to the front of the active list.
 */
	timersSetTimer(&clockTimers, tp, expTime, watchdog, NULL);
	nextTimeout = clockTimers->tmr_exp_time;
}

void resetTimer(Timer *tp) {
/* The timer pointed to by 'tp' is no longer needed. Remove it from both the
 * active and expired lists. Always update the next timeout time by setting 
 * it to the front of the active list.
 */
	timersClearTimer(&clockTimers, tp, NULL);
	nextTimeout = (clockTimers == NULL) ? TIMER_NEVER : clockTimers->tmr_exp_time;
}

unsigned long readClock() {
/* Read the counter of channel 0 of the 8253A timer. This counter counts
 * down at a rate of TIMER_FREQ and restarts at TIMER_COUNT-1 when it
 * reaches zero. A hardware interrupt (clock tick) occurs when the counter
 * gets to zero and restarts its cycle.
 */
	unsigned count;

	outb(TIMER_MODE, LATCH_COUNT);
	count = inb(TIMER0);
	count |= inb(TIMER0) << 8;

	return count;
}

void clockStop() {
/* Reset the clock to the BIOS rate. (For rebooting) */
	outb(TIMER_MODE, SQUARE_WAVE);
	outb(TIMER0, 0);
	outb(TIMER0, 0);
}


