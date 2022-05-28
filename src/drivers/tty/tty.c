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
unsigned long rsIrqSet = 0;

/* Address of a tty structure. */
#define ttyAddr(line)	(&ttyTable[line])

/* Macros for magic tty types. */
#define isConsole(tp)	((tp) < ttyAddr(NR_CONS))
#define isPty(tp)		((tp) >= ttyAddr(NR_CONS + NR_RS_LINES))

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
static struct WinSize winSizeDefaults;

/* Global variables for the TTY task (declared extern in tty.h). */
TTY ttyTable[NR_CONS + NR_RS_LINES + NR_PTYS];
int currConsIdx;		/* Currently active console index */
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
	if (tp->tty_in_cum >= tp->tty_min && tp->tty_in_left > 0) {
		if (tp->tty_in_rep_code == REVIVE) {
			notify(tp->tty_in_caller);
			tp->tty_in_revived = 1;
		} else {
			ttyReply(tp->tty_in_rep_code, tp->tty_in_caller,
					tp->tty_in_proc, tp->tty_in_cum);
			tp->tty_in_left = tp->tty_in_cum = 0;
		}
	}

	if (tp->tty_select_ops) 
	  selectRetry(tp);
#if NR_PTYS > 0
	if (isPty(tp))
	  selectRetryPty(tp);
#endif
}

static void ttyTimedout(Timer *tr) {
/* This timer has expired. Set the events flag, to force processing. */
	TTY *tp;

	tp = &ttyTable[timerArg(tr)->ta_int];
	tp->tty_min = 0;	/* Force read to succeed */
	tp->tty_events = 1;
}

static void expireTimers() {
/* A synchronous alarm message was received. Check if there are any expired
 * timers. Possibly set the event flag and reschedule another alarm.
 */
	clock_t now;	/* Current time */
	int s;

	/* Get the current time to compare the timers against. */
	if ((s = getUptime(&now)) != OK)
	  panic("TTY", "Couldn't get uptime from clock.", s);

	/* Scan the queue of timers for expired timers. This dispatch the watchdog
	 * functions of expired timers. Possibly a new alarm call must be scheduled.
	 */
	timersExpTimers(&ttyTimers, now, NULL);
	if (ttyTimers == NULL)
	  ttyNextTimeout = TIMER_NEVER;
	else {	/* Set new sync alarm */
		ttyNextTimeout = ttyTimers->tmr_exp_time;
		if ((s = sysSetAlarm(ttyNextTimeout, 1)) != OK)
		  panic("TTY", "Couldn't set synchronous alarm.", s);
	}
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

static void doStatus(Message *msg) {
//TODO
}

static void doRead(register TTY *tp, register Message *msg) {
/* A process wants to read from a terminal. */
	int r, status;
	phys_bytes physAddr;

	/* Check if there is already a process hanging in a read, check if the
	 * parameters are correct, do I/O.
	 */
	if (tp->tty_in_left > 0) {
		r = EIO;
	} else if (msg->COUNT <= 0) {
		r = EINVAL;
	} else if (sysUMap(msg->PROC_NR, D, (vir_bytes) msg->ADDRESS, 
					msg->COUNT, &physAddr) != OK) {
		r = EFAULT;
	} else {
		/* Copy information from the message to the tty struct. */
		tp->tty_in_rep_code = TASK_REPLY;
		tp->tty_in_caller = msg->m_source;
		tp->tty_in_proc = msg->PROC_NR;
		tp->tty_in_vir = (vir_bytes) msg->ADDRESS;
		tp->tty_in_left = msg->COUNT;

		if (!(tp->tty_termios.c_lflag & ICANON) &&
				tp->tty_termios.c_cc[VTIME] > 0) {
			if (tp->tty_termios.c_cc[VMIN] == 0) {
				/* MIN & TIME specify a read timer that finishes the
				 * read in TIME/10 seconds if no bytes are available.
				 */
				setTimer(tp, true);
				tp->tty_min = 1;
			} else {
				/* MIN & TIME specify an inter-byte timer that may
				 * have to be cancelled if there are no bytes yet.
				 */
				if (tp->tty_eot_count == 0) {
					setTimer(tp, false);
					tp->tty_min = tp->tty_termios.c_cc[VMIN];
				}
			}
		}

		/* Anything waiting in the input buffer? Clear it out... */
		inTransfer(tp);
		/* ...then go back for more. */
		handleEvents(tp);
		if (tp->tty_in_left == 0) {
			if (tp->tty_select_ops)
			  selectRetry(tp);
			return;		/* Already done. */
		}

		/* There were no bytes in the input queue available, so either suspend
		 * the caller or break off the read if nonblocking.
		 */
		if (msg->TTY_FLAGS & O_NONBLOCK) {
			r = EAGAIN;		/* Cancel the read */
			tp->tty_in_left = tp->tty_in_cum = 0;
		} else {
			r = SUSPEND;	/* Suspend the caller */
			tp->tty_in_rep_code = REVIVE;
		}
	}

	ttyReply(TASK_REPLY, msg->m_source, msg->PROC_NR, r);
	if (tp->tty_select_ops)
	  selectRetry(tp);
}

static void doWrite(register TTY *tp, register Message *msg) {
/* A process wants to write on a terminal. */
	int r;
	phys_bytes physAddr;

	/* Check if there is already a process hanging in a write, check if the
	 * parameters are correct, do I/O.
	 */
	if (tp->tty_out_left > 0) {
		r = EIO;
	} else if (msg->COUNT <= 0) {
		r = EINVAL;
	} else if (sysUMap(msg->PROC_NR, D, (vir_bytes) 
					msg->ADDRESS, msg->COUNT, &physAddr) != OK) {
		/* Copy message parameters to the tty structure. */
		tp->tty_out_rep_code = TASK_REPLY;
		tp->tty_out_caller = msg->m_source;
		tp->tty_out_proc = msg->PROC_NR;
		tp->tty_out_vir = (vir_bytes) msg->ADDRESS;
		tp->tty_out_left = msg->COUNT;

		/* Try to write. */
		handleEvents(tp);
		if (tp->tty_out_left == 0) 
		  return;		/* Already done. */

		/* None or not all the bytes could be written, so either suspend the
		 * caller or break off the write if nonblocking.
		 */
		if (msg->TTY_FLAGS & O_NONBLOCK) {	/* Cancel the write */
			r = tp->tty_out_cum > 0 ? tp->tty_out_cum : EAGAIN;
			tp->tty_out_left = tp->tty_out_cum = 0;
		} else {
			r = SUSPEND;	/* Suspend the caller */
			tp->tty_out_rep_code = REVIVE;
		}
	}
	ttyReply(TASK_REPLY, msg->m_source, msg->PROC_NR, r);
}

static void doIoctl(TTY *tp, Message *msg) {
//TODO
}

static void doOpen(register TTY *tp, Message *msg) {
/* A tty line has been opend. Make it the callers controlling tty if
 * O_NOCTTY is *not* set and it is not the log device. 1 is returned if
 * the tty is made the controlling tty, otherwise OK or an error code.
 */
	int r = OK;

	if (msg->TTY_LINE == LOG_MINOR) {
		/* The log device is a write-only diagnostics device. */
		if (msg->COUNT & R_BIT)
		  r = EACCES;
	} else {
		if (! (msg->COUNT & O_NOCTTY)) {
			tp->tty_pgrp = msg->PROC_NR;
			r = 1;
		}
		++tp->tty_open_count;
	}
	ttyReply(TASK_REPLY, msg->m_source, msg->PROC_NR, r);
}

static void ttyInputCancel(register TTY *tp) {
/* Discard all pending input, tty buffer or device. */

	tp->tty_in_count = tp_tty_eot_count = 0;
	tp->tty_in_tail = tp->tty_in_head;
	(*tp->tty_in_cancel)(tp, 0);
}

static void doClose(register TTY *tp, Message *msg) {
/* A tty line has been closed. Clean up the line if it is the last close. */

	if (msg->TTY_LINE != LOG_MINOR && --tp->tty_open_count == 0) {
		tp->tty_pgrp = 0;
		ttyInputCancel(tp);
		(*tp->tty_out_cancel)(tp, 0);
		(*tp->tty_close)(tp, 0);
		tp->tty_termios = termiosDefaults;
		tp->tty_win_Size = winSizeDefaults;
		setAttr(tp);
	}
	ttyReply(TASK_REPLY, msg->m_source, msg->PROC_NR, OK);
}

static void doSelect(TTY *tp, Message *msg) {
//TODO
}

static void doCancel(register TTY *tp, Message *msg) {
/* A signal has been sent to a process that is hanging trying to read or write.
 * The pending read or write must be finished off immediately.
 */
	int pNum;
	int mode;

	/* Check the parameters carefully, to avoid cancelling twice. */
	pNum = msg->PROC_NR;
	mode = msg->COUNT;
	if ((mode & R_BIT) && tp->tty_in_left != 0 && pNum == tp->tty_in_proc) {
		/* Process was reading when killed. Clean up input. */
		ttyInputCancel(tp);
		tty->tty_in_left = tp->tty_in_cum = 0;
	}
	if ((mode & W_BIT) && tp->tty_out_left != 0 && pNum == tp->tty_out_proc) {
		/* Process was writing when killed. Clean up output. */
		(*tp->tty_out_cancel)(tp, 0);
		tp->tty_out_left = tp->tty_out_cum = 0;
	}
	if (tp->tty_io_req != 0 && pNum == tp->tty_io_proc) {
		/* Process was waiting for output to drain. */
		tp->tty_io_req = 0;
	}
	tp->tty_events = 1;
	ttyReply(TASK_REPLY, msg->m_source, pNum, EINTR);
}

void main() {
/* Main routine of the terminal task. */

	Message msg;		/* Buffer for all incoming messages */
	unsigned line;
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

		/* Get a request message. */
		receive(ANY, &msg);

		/* First handle all kernel notification types that the TTY supports.
		 *  - An alarm went off, expire all timers and handle the events.
		 *  - A hardware interrupt also is an invitation to check for events.
		 *  - A new kernel message is available for printing.
		 *  - Reset the console on system shutdown.
		 * Then see if this message is different from a normal device driver
		 * request and should be handled separately. These extra functions
		 * do not operate on a device, in constrast to the driver requests.
		 */
		switch (msg.m_type) {
			case SYN_ALARM:		
				expireTimers();	/* Run watchdogs of expired timers */
				continue;		/* Continue to check for events */
			case HARD_INT: {	/* Hardware interrupt notification */
				if (msg.NOTIFY_ARG & kbdIrqSet)
				  kbdInterrupt(&msg);	/* Fetch chars from keyboard */
#if NR_RS_LINES > 0
				if (msg.NOTIFY_ARG & rsIrqSet)
				  rsInterrupt(&msg);	/* Serial I/O */
#endif
				expireTimers();	/* Run watchdogs of expired timers */
				continue;		/* Continue to check for events */
			}
			case SYS_SIG: {		/* System signal */
				sigset_t sigset = (sigset_t) msg.NOTIFY_ARG;
				if (sigismember(&sigset, SIGKSTOP)) {
					consoleStop();	/* Switch to primary console */
					if (irqHookId != -1) {
						sysIrqDisable(&irqHookId);
						sysIrqRmPolicy(KEYBOARD_IRQ, &irqHookId);
					}
				}
				if (sigismember(&sigset, SIGTERM))
				  consoleStop();
				if (sigismember(&sigset, SIGKMESS))
				  doNewKernelMsg(&msg);
				continue;
			}
			case PANIC_DUMPS:	/* Allow panic dumps */
				consoleStop();	/* Switch to primary console */
				doPanicDumps(&msg);
				continue;
			case DIAGNOSTICS:	/* A server wants to print some */
				doDiagnostics(&msg);
				continue;
			case FKEY_CONTROL:	/* (un)register a fkey observer */
				doFKeyCtl(&msg);
				continue;
			default:			/* Should be a driver request */
				;
		}
	
		/* Only device requests should get to this point. All requests,
		 * except DEV_STATUS, have a minor device number. Check this
		 * exception and get the minor device number otherwise.
		 */
		if (msg.m_type == DEV_STATUS) {
			doStatus(&msg);
			continue;
		}
		line = msg.TTY_LINE;
		if ((line - CONS_MINOR) < NR_CONS) {
			tp = ttyAddr(line - CONS_MINOR);
		} else if (line == LOG_MINOR) {
			tp = ttyAddr(0);
		} else if ((line - RS232_MINOR) < NR_RS_LINES) {
			tp = ttyAddr(line - RS232_MINOR + NR_CONS);
		} else if ((line - TTYPX_MINOR) < NR_PTYS) {
			tp = ttyAddr(line - TTYPX_MINOR + NR_CONS + RS232_MINOR);
		} else if ((line - PTYPX_MINOR) < NR_PTYS) {
			tp = ttyAddr(line - PTYPX_MINOR + NR_CONS + RS232_MINOR);
			if (msg.m_type != DEV_IOCTL) {
				doPty(tp, &msg);
				continue;
			}
		} else {
			tp = NULL;
		}

		/* If the device doesn't exist or is not configured return ENXIO. */
		if (tp == NULL || ! ttyActive(tp)) {
			printf("Warning, TTY got illegal request %d from %d\n",
					msg.m_type, msg.m_source);
			ttyReply(TASK_REPLY, msg.m_source, msg.PROC_NR, ENXIO);
			continue;
		}

		/* Execute the requested device driver function. */
		switch (msg.m_type) {
			case DEV_READ:
				doRead(tp, &msg);
				break;
			case DEV_WRITE:
				doWrite(tp, &msg);
				break;
			case DEV_IOCTL:
				doIoctl(tp, &msg);
				break;
			case DEV_OPEN:
				doOpen(tp, &msg);
				break;
			case DEV_CLOSE:
				doClose(tp, &msg);
				break;
			case DEV_SELECT:
				doSelect(tp, &msg);
				break;
			case CANCEL:
				doCancel(tp, &msg);
				break;
			default:
				printf("Warning, TTY got unexpected request %d from %d\n",
						msg.m_type, msg.m_source);
				ttyReply(TASK_REPLY, msg.m_source, msg.PROC_NR, EINVAL);
		}
	}
}





