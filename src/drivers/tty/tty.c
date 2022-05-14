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

void ttyReply(int code, int replyee, int pNum, int status) {
/* Send a reply to a process that wanted to read or write data. */
	Message msg;
	
	msg.m_type = code;
	msg.REP_PROC_NR = pNum;
	msg.REP_STATUS = status;

	if ((status = send(replyee, &msg)) != OK) 
	  panic("TTY", "ttyReply failed, status\n", status);
}

static void inTransfer(register TTY *tp) {
/* Transfer bytes from the input queue to a process reading from a terminal. */
	int ch;
	int count;
	char buf[64], *bp;

	/* Force read to succeed if the line is hung up, looks like EOF to reader. */
	if (tp->tty_termios.c_ospeed == B0)
	  tp->tty_min = 0;

	/* Anything to do? */
	if (tp->tty_in_left == 0 || tp->tty_eot_count < tp->tty_min)
	  return;

	bp = buf;
	while (tp->tty_in_left > 0 && tp->tty_eot_count > 0) {
		ch = *tp->tty_in_tail;

		if (!(ch & IN_EOF)) {
			/* One character to be delivered to the user. */
			*bp = ch & IN_CHAR;
			--tp->tty_in_left;
			if (++bp == bufEnd(buf)) {
				/* Temp buffer full, copy to user space. */
				sysVirCopy(SELF, D, (vir_bytes) buf,
					tp->tty_in_proc, D, tp->tty_in_vir, 
					(vir_bytes) bufLen(buf));
				tp->tty_in_vir += bufLen(buf);
				tp->tty_in_cum += bufLen(buf);
				bp = buf;
			}
		}

		/* Remove the character from the input queue. */
		if (++tp->tty_in_tail == bufEnd(tp->tty_in_buf))
		  tp->tty_in_tail = tp->tty_in_buf;
		--tp->tty_in_count;
		if (ch & IN_EOT) {
			--tp->tty_eot_count;
			/* Don't read past a line break in canonical mode. */
			if (tp->tty_termios.c_lflag & ICANON)
			  tp->tty_in_left = 0;
		}
	}

	if (bp > buf) {
		/* Leftover characters in the buffer. */
		count = bp - buf;
		sysVirCopy(SELF, D, (vir_bytes) buf, 
			tp->tty_in_proc, D, tp->tty_in_vir, (vir_bytes) count);
		tp->tty_in_vir += count;
		tp->tty_in_cum += count;
	}

	/* Usually reply to the reader, possibly even if in_cum == 0 (EOF). */
	if (tp->tty_in_left == 0) {
		if (tp->tty_in_rep_code == REVIVE) {
			notify(tp->tty_in_caller);
			tp->tty_in_revived = 1;
		} else {
			ttyReply(tp->tty_in_rep_code, tp->tty_in_caller,
						tp->tty_in_proc, tp->tty_in_cum);
			tp->tty_in_left = tp->tty_in_cum = 0;
		}
	}
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

static void ttyTimedout(Timer *tr) {
/* This timer has expired. Set the events flag, to force processing. */
	TTY *tp;

	tp = &ttyTable[timerArg(tr)->ta_int];
	tp->tty_min = 0;	/* Force read to succeed */
	tp->tty_events = 1;
}

static void setTimer(TTY *tp, bool enable) {
	clock_t now, expTime;
	int s;

	/* Get the current time to calculate the timeout time. */
	if ((s = getUptime(&now)) != OK) 
	  panic("TTY", "Couldn't get uptime from clock.", s);
	
	if (enable) {
		expTime = now + tp->tty_termios.c_cc[VTIME] * (HZ / 10);
		/* Set a new timer for enabling the TTY events flags. */
		timersSetTimer(&ttyTimers, &tp->tty_timer, 
				expTime, ttyTimedout, NULL);
	} else {
		/* Remove the timer from the active and expired lists. */
		timersClearTimer(&ttyTimers, &tp->tty_timer, NULL);
	}

	/* Now check if a new alarm must be scheduled. This happens when the front
	 * of the timers queue was disabled or reinserted at another position, or
	 * when a new timer was added to the front.
	 */
	if (ttyTimers == NULL)
	  ttyNextTimeout = TIMER_NEVER;
	else if (ttyTimers->tmr_exp_time != ttyNextTimeout) {
		ttyNextTimeout = ttyTimers->tmr_exp_time;
		if ((s = sysSetAlarm(ttyNextTimeout, 1)) != OK)
		  panic("TTY", "Couldn't set synchronous alarm.", s);
	}
}

static void rawEcho(register TTY *tp, int ch) {
/* Echo without interpretation if ECHO is set. */
	int rp = tp->tty_reprint;
	if (tp->tty_termios.c_lflag & ECHO)
	  (*tp->tty_echo)(tp, ch);
	tp->tty_reprint = rp;
}

static int ttyEcho(register TTY *tp, register int ch) {
/* Echo the character if echoing is on. Some control characters are echoed
 * with their normal effect, other control characters are echoed as "^X",
 * normal characters are echoed normally. EOF (^D) is echoed, but immediately
 * backspaced over. Return the character with the echoed length added to its
 * attributes.
 */
	int len, rp;

	ch &= ~IN_LEN;
	if (!(tp->tty_termios.c_lflag & ECHO)) {
		if (ch == ('\n' | IN_EOT) && 
			(tp->tty_termios.c_lflag & (ICANON | ECHONL)) == (ICANON | ECHONL)) 
		  (*tp->tty_echo)(tp, '\n');
		return ch;
	}

	/* "Reprint" tells if the echo output has been meesed up by other output. */
	rp = tp->tty_in_count == 0 ? false : tp->tty_reprint;

	if ((ch & IN_CHAR) < ' ') {
		switch (ch & (IN_ESC | IN_EOF | IN_EOT | IN_CHAR)) {
			case '\t':
				len = 0;
				do {
					(*tp->tty_echo)(tp, ' ');
					++len;
				} while (len < TAB_SIZE && (tp->tty_position & TAB_MASK) != 0);
				break;
			case '\r' | IN_EOT:
			case '\n' | IN_EOT:
				(*tp->tty_echo)(tp, ch & IN_CHAR);
				len = 0;
				break;
			default:
				(*tp->tty_echo)(tp, '^');
				(*tp->tty_echo)(tp, '@' + (ch & IN_CHAR));
				len = 2;
		}
	} else if ((ch & IN_CHAR) == '\177') {
		/* A DEL prints as "^?". */
		(*tp->tty_echo)(tp, '^');
		(*tp->tty_echo)(tp, '?');
		len = 2;
	} else {
		(*tp->tty_echo)(tp, ch & IN_CHAR);
		len = 1;
	}
	if (ch & IN_EOF) {
		while (len > 0) {
			(*tp->tty_echo)(tp, '\b');
			--len;
		}
	}

	tp->tty_reprint = rp;

	return (ch | (len << IN_LSHIFT));
}

static void reprint(register TTY *tp) {
/* Restore what has been echoed to screen before if the user input has been
 * messed up by output, or if REPRINT (^R) is typed.
 */
	int count;
	u16_t *head;

	tp->tty_reprint = false;
	
	/* Find the last time break in the input. */
	head = tp->tty_in_head;
	count = tp->tty_in_count;
	while (count > 0) {
		if (head == tp->tty_in_buf)
		  head = bufEnd(tp->tty_in_buf);
		if (head[-1] & IN_EOT)
		  break;
		--head;
		--count;
	}
	if (count == tp->tty_in_count)
	  return;	/* No reason to reprint */

	/* Show REPRINT (^R) and move to a new line. */
	ttyEcho(tp, tp->tty_termios.c_cc[VREPRINT] | IN_ESC);
	rawEcho(tp, '\r');
	rawEcho(tp, '\n');

	/* Reprint from the last break onwards. */
	do {
		if (head == bufEnd(tp->tty_in_buf))
		  head = tp->tty_in_buf;
		*head = ttyEcho(tp, *head);
		++head;
		++count;
	} while (count < tp->tty_in_count);
}

static int backOver(register TTY *tp) {
/* Backspace to previous character on screen and earse it. */
	u16_t *head;
	int len;

	if (tp->tty_in_count == 0)
	  return 0;		/* Queue empty */
	head = tp->tty_in_head;
	if (head == tp->tty_in_buf)
	  head = bufEnd(tp->tty_in_buf);
	if (*--head & IN_EOT)
	  return 0;		/* Can't earse "line breaks" */
	if (tp->tty_reprint)
	  reprint(tp);	/* Reprint if messed up */
	tp->tty_in_head = head;
	--tp->tty_in_count;
	if (tp->tty_termios.c_lflag & ECHOE) {
		len = (*head & IN_LEN) >> IN_LSHIFT;
		while (len > 0) {
			rawEcho(tp, '\b');
			rawEcho(tp, ' ');
			rawEcho(tp, '\b');
			--len;
		}
	}
	return 1;	/* One character erased */
}

int inProcess(register TTY *tp, char *buf, int count) {
/* Characters have just been typed in. Process, save, and echo them. 
 * Return the number of characters processed.
 */
	int ch, sig, ct;
	bool timeSet = false;

	for (ct = 0; ct < count; ++ct) {
		/* Take one character. */
		ch = *buf++ & BYTE;
	
		/* Strip to seven bits? */
		if (tp->tty_termios.c_iflag & ISTRIP)
		  ch &= 0x7F;

		/* Input extensions? */
		if (tp->tty_termios.c_lflag & IEXTEN) {
			/* Previous character was a character escape? */
			if (tp->tty_escaped) {
				tp->tty_escaped = NOT_ESCAPED;
				ch |= IN_ESC;	/* Protect character */
			}

			/* LNEXT (^V) to escape the next character? */
			if (ch == tp->tty_termios.c_cc[VLNEXT]) {
				tp->tty_escaped = ESCAPED;
				rawEcho(tp, '^');
				rawEcho(tp, '\b');	/* move the cursor under '^' */
				continue;	/* Do not store the escape */
			}

			/* REPRINT (^R) to reprint echoed characters? */
			if (ch == tp->tty_termios.c_cc[VREPRINT]) {
				reprint(tp);
				continue;
			}
		}

		/* _POSIX_VDISABLE is a normal character value, so better escape it. */
		if (ch == _POSIX_VDISABLE) 
		  ch |= IN_ESC;

		/* Map CR to LF, ignore CR, or map LF to CR. */
		if (ch == '\r') {
			if (tp->tty_termios.c_iflag & IGNCR)
			  continue;
			if (tp->tty_termios.c_iflag & ICRNL)
			  ch = '\n';
		} else if (ch == '\n') {
			if (tp->tty_termios.c_iflag & INLCR)
			  ch = '\r';
		}

		/* Canonical mode? */
		if (tp->tty_termios.c_lflag & ICANON) {
			/* Erase processing (rub out of last character). */
			if (ch == tp->tty_termios.c_cc[VERASE]) {
				backOver(tp);
				if (!(tp->tty_termios.c_lflag & ECHOE))
				  ttyEcho(tp, ch);
				continue;
			}

			/* Kill processing (remove current line). */
			if (ch == tp->tty_termios.c_cc[VKILL]) {
				while (backOver(tp)) {
				}
				if (!(tp->tty_termios.c_lflag & ECHOE)) {
					ttyEcho(tp, ch);
					if (tp->tty_termios.c_lflag & ECHOK)
					  rawEcho(tp, '\n');
				}
				continue;
			}

			/* EOF (^D) means end-of-file, an invisible "line break". */
			if (ch == tp->tty_termios.c_cc[VEOF])
			  ch |= IN_EOT | IN_EOF;

			/* The line may be returned to the user after an LF. */
			if (ch == '\n')
			  ch |= IN_EOT;

			/* Same thing with EOL, whatever it may be. */
			if (ch == tp->tty_termios.c_cc[VEOL])
			  ch |= IN_EOT;
		}

		/* Start/stop input control? */
		if (tp->tty_termios.c_iflag & IXON) {
			/* Output stops on STOP (^S). */
			if (ch == tp->tty_termios.c_cc[VSTOP]) {
				tp->tty_inhibited = STOPPED;
				tp->tty_events = 1;
				continue;
			}

			/* Output restarts on START (^Q) or any character if IXANY. */
			if (tp->tty_inhibited) {
				if (ch == tp->tty_termios.c_cc[VSTART] ||
						(tp->tty_termios.c_iflag & IXANY)) {
					tp->tty_inhibited = RUNNING;
					tp->tty_events = 1;
					if (ch == tp->tty_termios.c_cc[VSTART])
					  continue;
				}
			}
		}

		if (tp->tty_termios.c_lflag & ISIG) {
			/* Check for INTR (^?) and QUIT (^\) characters. */
			if (ch == tp->tty_termios.c_cc[VINTR] ||
					ch == tp->tty_termios.c_cc[VQUIT]) {
				sig = SIGINT;
				if (ch == tp->tty_termios.c_cc[VQUIT])
				  sig = SIGQUIT;
				signalChar(tp, sig);
				ttyEcho(tp, ch);
				continue;
			}
		}

		/* Is there space in the input buffer? */
		if (tp->tty_in_count == bufLen(tp->tty_in_buf)) {
			/* No space; discard in canonical mode, keep in raw mode. */
			if (tp->tty_termios.c_lflag & ICANON)
			  continue;
			break;
		}

		if (!(tp->tty_termios.c_lflag & ICANON)) {
			/* In raw mode all characters are "line breaks". */
			ch |= IN_EOT;

			/* Start an inter-byte timer? */
			if (!timeSet && tp->tty_termios.c_cc[VMIN] > 0 &&
					tp->tty_termios.c_cc[VTIME] > 0) {
				setTimer(tp, true);
				timeSet = true;
			}
		}

		/* Perform the intricate function of echoing. */
		if (tp->tty_termios.c_lflag & (ECHO | ECHONL))
		  ch = ttyEcho(tp, ch);

		/* Save the character in the input queue. */
		*tp->tty_in_head++ = ch;
		if (tp->tty_in_head == bufEnd(tp->tty_in_buf))
		  tp->tty_in_head = tp->tty_in_buf;
		++tp->tty_in_count;
		if (ch & IN_EOT)
		  ++tp->tty_eot_count;

		/* Try to finish input if the queue threatens to overflow. */
		if (tp->tty_in_count == bufLen(tp->tty_in_buf))
		  inTransfer(tp);
	}
	return ct;
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





