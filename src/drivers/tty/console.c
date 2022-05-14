#include "../drivers.h"
#include "termios.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "tty.h"

#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"

typedef struct {
	TTY *c_tty;		/* Associated TTY struct */
	int c_column;	/* Current column number (0-origin) */
	int c_row;		/* Current row (0 at top of screen) */

} Console;

static int numConsoles = 1;		/* Actual number of consoles */
static Console consoleTable[NR_CONS];
static Console *currConsole;	/* Currently visible */

void screenInit(TTY *tp) {
/* Initialize the screen driver. */
	Console *console;
	int line;

	/* Associate console and TTY. */
	line = tp - &ttyTable[0];
	if (line >= numConsoles)
	  return;
	console = &consoleTable[line];
	console->c_tty = tp;
	tp->tty_priv = console;

	/* Initialize the keyboard driver. */
	kbInit(tp);

	/* Fill in TTY function hooks. */
	tp->tty_dev_write = consoleWrite;
	tp->tty_echo = consoleEcho;
	tp->tty_ioctl = consoleIoctl;

	/* Get the BIOS parameters that describe the VDU. */
}
