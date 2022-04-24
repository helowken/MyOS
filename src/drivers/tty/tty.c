#include "../drivers.h"
#include "termios.h"
#include "sys/ioc_tty.h"
#include "signal.h"
#include "minix/callnr.h"
#include "minix/keymap.h"
#include "tty.h"

#include "sys/time.h"
#include "sys/select.h"

extern int irqHookId;

unsigned long kbdIrqSet = 0;

/* Address of a tty structure. */
#define ttyAddr(line)	(&ttyTable[line])

/* Macros for magic tty structure pointers. */
#define FIRST_TTY	ttyAddr(0)
#define END_TTY		ttyAddr(sizeof(ttyTable) / sizeof(ttyTable[0]))

/* A device exists if at least its 'devRead' function is defined. */
#define ttyActive(tp)	((tp)->tty_dev_read != NULL)

/* Default attributes. */
static struct termios termiosDefaults = {
	TINPUT_DEF, TOUTPUT_DEF, TCTRL_DEF, TLOCAL_DEF,
	TSPEED_DEF, TSPEED_DEF,
	{
		TEOF_DEF, TEOL_DEF, TERASE_DEF, TINTR_DEF,
		TKILL_DEF, TMIN_DEF, TQUIT_DEF, TTIME_DEF,
		TSUSP_DEF, TSTART_DEF, TSTOP_DEF, TREPRINT_DEF,
		TLNEXT_DEF, TDISCARD_DEF
	}
};

/* Global variables for the TTY task (declared extern in tty.h). */
TTY ttyTable[NR_CONS + NR_RS_LINES + NR_PTYS];
int currConsole;		/* Currently active console */
Timer *ttyTimers;		/* Queue of TTY timers */
Machine machine;		/* Kernel environment variables */


int ttyDevNop(TTY *tp, int try) {
/* Some functions need not be implemented at the device level. */
	return 0;
}

static void ttyInit() {
/* Initialize tty structure and call device initialization routines. */
	
	register TTY *tp;
	int s;

	/* Intialize the terminal lines. */
	for (tp = FIRST_TTY, s = 0; tp < END_TTY; ++tp, ++s) {
		tp->tty_index = s;

		timerInit(&tp->tty_timer);

		tp->tty_in_tail = tp->tty_in_head = tp->tty_in_buf;
		tp->tty_min = 1;
		tp->tty_termios = termiosDefaults;
		tp->tty_in_cancel = tp->tty_out_cancel = tp->tty_ioctl = tp->tty_close = ttyDevNop;

		if (tp < ttyAddr(NR_CONS)) {
			screenInit(tp);
			tp->tty_minor = CONS_MINOR + s;
		} else if (tp < ttyAddr(NR_CONS + NR_RS_LINES)) {
			rsInit(tp);
			tp->tty_minor = RS232_MINOR + s - NR_CONS;
		} else {
			ptyInit(tp);
			tp->tty_minor = s - (NR_CONS + NR_RS_LINES) + TTYPX_MINOR;
		}
	}
}

static void devIoctl(TTY *tp) {
	// TODO
}

static void inTransfer(register TTY *tp) {
	// TODO
}

void handleEvents(TTY *tp) {
/* Handle any events pending on a TTY. These events are usually device
 * interrupts.
 *
 * Two kinds of events are prominent:
 *  - a character has been received from the console or an RS232 line.
 *  - an RS232 line has completed a write request (on behalf of a user).
 *  The interrupt handler may delay the interrupt message at its discretion
 *  to avoid swamping the TTY task. Messages may be overwritten when the
 *  lines are fast or when there are races between different lines, input
 *  and output, because MINIX only provides single buffering for interrupt
 *  messages (in proc.c). This is handled by explicityly checking each line
 *  for fresh input and completed output on each interrupt.
 */
	//unsigned count;

	do {
		tp->tty_events = 0;

		/* Read input and perform input processing. */
		(*tp->tty_dev_read)(tp, 0);

		/* Perform output processing and write output. */
		(*tp->tty_dev_write)(tp, 0);

		/* Ioctl waiting for some event? */
		if (tp->tty_io_req != 0)
		  devIoctl(tp);
	} while (tp->tty_events);

	/* Transfer characters from the input queue to a waiting process. */
	inTransfer(tp);

	/* Reply if enough bytes are available. */
	// TODO
}

void main() {
/* Main routine of the terminal task. */

	//Message ttyMsg;		/* Buffer for all incoming messages */
	//unsigned line;
	int s;
	register TTY *tp;

	/* Initialize the TTY driver. */
	ttyInit();

	/* Get kernel environment (protected_mode, pc_at and ega are needed). */
	if ((s = sysGetMachine(&machine)) != OK)
	  panic("TTY", "Couldn't obtain kernel environment.", s);
	
	/* First one-time keyboard initialization. */
	kbInitOnce();
	
	printf("\n");

	while (true) {
		/* Check for and handle any events on any of the ttys. */
		for (tp = FIRST_TTY; tp < END_TTY; ++tp) {
			if (tp->tty_events)
			  handleEvents(tp);
		}
	}
}





