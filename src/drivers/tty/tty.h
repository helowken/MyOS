#include "timers.h"

/* First minor numbers for the various classes of TTY devices. */
#define CONS_MINOR		0
#define LOG_MINOR		15
#define RS232_MINOR		16
#define TTYPX_MINOR		128
#define PTYPX_MINOR		192

#define	TTY_IN_BYTES	256		/* TTY input queue size */

struct TTY;
typedef int (*DevFunc)(struct TTY *tp, int tryOnly);
typedef void (*DevFuncArg)(struct TTY *tp, int c);

typedef struct TTY {
	int tty_events;		/* Set when TTY should inspect this line */
	int tty_index;		/* Index into TTY table */
	int tty_minor;		/* Device minor number */

	/* Input queue. Typed character are stored here until read by a program. */
	u16_t *tty_in_head;		/* Pointer to place where next char goes */
	u16_t *tty_in_tail;		/* Pointer to next char to be given to program */
	int tty_in_count;		/* # chars in the input queue */
	int tty_eot_count;		/* number of "line breaks" in input queue */
	DevFunc tty_dev_read;	/* Routine to read from low level buffers */
	DevFunc tty_in_cancel;	/* Cancel any device input */
	int tty_min;			/* Minimum requested #chars in input queue */
	Timer tty_timer;		/* The timer for this tty */

	/* Ouptut section. */
	DevFunc tty_dev_write;	/* Routine to start actual device output */
	DevFuncArg tty_echo;	/* Routine to echo characters input */
	DevFunc tty_out_cancel;	/* Cancel any ongoing device output */
	DevFunc tty_break;		/* Let the device send a break */

	/* Terminal parameters and status. */
	char tty_escaped;		/* 1 when LNEXT (^V) just seen, else 0 */
	char tty_inhibited;		/* 1 when STOP (^S) just seen (stops output) */

	/* Information about incomplete I/O requests is stored here. */
	char tty_in_proc;		/* Process that wants to read from tty */
	vir_bytes tty_in_vir;	/* Virtual address where data is to go */
	char tty_io_req;		/* ioctl request code */
	int tty_in_left;		/* How many chars are still needed */
	int tty_in_cum;			/* # chars input so far */

	/* select() data */
	int tty_select_ops;		/* Which operations are interesting */
	int tty_select_proc;	/* Which process wants notification */

	/* Miscellaneous. */
	DevFunc tty_ioctl;		/* Set line speed, etc. at the device level */
	DevFunc tty_close;		/* Tell the device that the tty is closed */
	void *ttpPriv;			/* Pointer to per device private data */
	struct termios tty_termios;		/* Terminal attributes */
	WinSize tty_win_size;	/* Window size (#lines and #columns) */

	u16_t tty_in_buf[TTY_IN_BYTES];	/* TTY input buffer */
} TTY;

/* Values for the fields. */
#define NOT_ESCAPED		0	/* Previous character is not LNEXT (^V) */
#define ESCAPED			1	/* Previous character was LNEXT (^V) */
#define RUNNING			0	/* No STOP (^S) has been typed to stop output */
#define STOPPED			1	/* STOP (^S) has been typed to stop output */

/* Fields and flags on characters in the input queue. */
#define IN_CHAR		0x00FF	/* Low 8 bits are the character itself */
#define IN_EOT		0x1000	/* Char is a line break (^D, LF) */
#define IN_EOF		0x2000	/* Char is EOF (^D), do not return to user */
#define IN_ESC		0x4000	/* Escaped by LNEXT (^V), no interpretation */


/* Memory allocated in tty.c, so extern here. */
extern TTY ttyTable[NR_CONS + NR_RS_LINES + NR_PTYS];
extern int currConsole;		/* Currently visible console */
extern int irqHookId;		/* Hook id for keyboard irq */

extern Timer *ttyTimers;	/* Queue of TTY timers */
extern clock_t ttyNextTimeout;	/* Next TTY timeout */

extern unsigned long kbdIrqSet;
extern Machine machine;		/* Machine information (a.o.: pc_at, ega) */

/* Number of elements and limit of a buffer. */
#define bufLen(buf)		(sizeof(buf) / sizeof((buf)[0]))
#define bufEnd(buf)		((buf) + bufLen(buf))

/* tty.c */
void signalChar(TTY *tp, int sig);
int inProcess(TTY *tp, char *buf, int count);
int selectRetry(TTY *tp);

/* rs232.c */
void rsInit(TTY *tp);

/* console.c */
void screenInit(TTY *tp);

/* keyboard.c */
void kbInit();
void kbInitOnce();
void kbdInterrupt();

/* pty.c */
void ptyInit(TTY *tp);

