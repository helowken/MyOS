#include "../drivers.h"
#include "termios.h"
#include "sys/ioc_tty.h"
#include "signal.h"
#include "minix/callnr.h"
#include "minix/keymap.h"
#include "tty.h"

#include "sys/time.h"
#include "sys/select.h"

/* Address of a tty structure. */
#define ttyAddr(line)	(&ttyTable[line])

/* Macros for magic tty structure pointers. */
#define FRIST_TTY	ttyAddr(0)
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

int ttyDevNop(TTY *tp, int try) {
/* Some functions need not be implemented at the device level. */
}

static void ttyInit() {
/* Initialize tty structure and call device initialization routines. */
	
	register TTY *tp;
	int s;
	struct sigaction sa;

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
		} else if (tp < ttpAddr(NR_CONS + NR_RS_LINES)) {
			rsInit(tp);
			tp->tty_minor = RS232_MINOR + s - NR_CONS;
		} else {
			ptyInit(tp);
			tp->tty_minor = s - (NR_CONS + NR_RS_LINES) + TTYPX_MINOR;
		}
	}
}

static void kbWait() {

}

static void setLeds() {
/* set the LEDs on the caps, num, and scroll lock keys */
	int s;
	if (!machine.pc_at) 
	  return;	/* PC/XT doesn't have LEDs */

	kbWait();	/* Wait for buffer empty */

}

static void kbInitOnce() {
	int i;
	
	setLeds();			/* Turn off numlock led */
	scanKeyboard();		/* Discard leftover keystroke */

	/* Clear the function key observers array. Also see funcKey(). */
	for (i = 0; i < 12; ++i) {
		
	}
}

void main() {
/* Main routine of the terminal task. */

	Message ttyMsg;		/* Buffer for all incoming messages */
	unsigned line;
	int s;

	/* Initialize the TTY driver. */
	ttyInit();

	/* Get kernel environment (protected_mode, pc_at and ega are needed). */
	if ((s = sysGetMachine(&machine)) != OK)
	  panic("TTY", "Couldn't obtain kernel environment.", s);
	
	/* First one-time keyboard initialization. */
	kbInitOnce();

}
