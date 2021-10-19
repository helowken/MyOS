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
#include "minix/com.h"

/* Clock parameters. */
#define SQUARE_WAVE		0x36	/* Channel=0, Access=low byte + high byte, 
								   Operating mode=square wave generator, 16-bit binary. */
#define TIMER_COUNT	((unsigned) (TIMER_FREQ / HZ))	/* Initial value for counter */
#define TIMER_FREQ	1193182L	/* Clock frequency for timer in PC and AT */

/* When a timer expires its watchDog function is run by the CLOCK task. */
static Timer *clockTimers;		/* Queue of CLOCK timers */
static clock_t nextTimoout;		/* Realtime that next timer expires */

/* The time is incremented by the interrupt handler on each clock tick. */
static clock_t realTime;		/* Real time clock */
static IRQHook clockHook;		/* Interrupt handler hook */


/* Main program of clock task. if the call is not HARD_INT it is an error. */


