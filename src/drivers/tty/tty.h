#include "timers.h"

/* First minor numbers for the various classes of TTY devices. */
#define CONS_MINOR		0
#define LOG_MINOR		15
#define RS232_MINOR		16
#define TTYPX_MINOR		128
#define PTYPX_MINOR		192

#define	TTY_IN_BYTES	256		/* TTY input queue size */

struct TTY;
typedef int (*DevFunc)(struct tty *tp, int tryOnly);
typedef void (*DevFuncArg)(struct tty *tp, int c);

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
	DevFunc tty_dev_write;		/* Routine to start actual device output */
	DevFuncArg tty_echo;		/* Routine to echo characters input */
	DevFunc tty_out_cancel;		/* Cancel any ongoing device output */
	DevFunc tty_break;			/* Let the device send a break */

	/* Miscellaneous. */
	DevFunc tty_ioctl;		/* Set line speed, etc. at the device level */
	DevFunc tty_close;		/* Tell the device that the tty is closed */
	void *ttpPriv;			/* Pointer to per device private data */
	struct termios tty_termios;		/* Terminal attributes */
	struct WinSize tty_win_size;	/* Window size (#lines and #columns) */

	u16_t tty_in_buf[TTY_IN_BYTES];	/* TTY input buffer */
} TTY;

/* Memory allocated in tty.c, so extern here. */
extern TTY ttyTable[NR_CONS + NR_RS_LINES + NR_PTYS];
extern int currConsole;		/* Currently visible console */
extern int irqHookId;		/* Hook id for keyboard irq */


