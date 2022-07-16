#include "../drivers.h"
#include "termios.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "tty.h"

#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"

/* Definitions used by the console driver. */
#define MONO_BASE		0xB0000L	/* Base of mono video memory */
#define COLOR_BASE		0xB8000L	/* Base of color video memory */
#define MONO_SIZE		0x1000		/* 4K mono video memory */
#define COLOR_SIZE		0x4000		/* 16K color video memory */
#define EGA_SIZE		0x8000		/* EGA & VGA have at least 32K */
#define BLANK_COLOR		0x0700		/* Determines cursor color on blank screen */
#define SCROLL_UP			0		/* Scroll forward */
#define SCROLL_DOWN			1		/* Scroll backward */
#define BLANK_MEM	((u16_t *) 0)	/* Tells mem2VidCopy() to blank the screen */
#define CONS_RAM_WORDS		80		/* Video ram buffer size */
#define MAX_ESC_PARAMS		4		/* Number of escape sequence params allowed */

/* Constants relating to the controller chips. */
#define	PORT_M_6845		0x3B4	/* Port for 6845 mono */
#define PORT_C_6845		0x3D4	/* Port for 6845 color */
#define REG_INDEX		0		/* 6845's index register */
#define REG_DATA		1		/* 6845's data register */
#define REG_STATUS		6		/* 6845's status register */
#define REG_VID_ORG		12		/* 6845's origin register */
#define REG_CURSOR		14		/* 6845's cursor register */

/* Global variables used by the console driver and assembly support. */
int vidSegIdx;		/* Index of video segment in remote memory map */
u16_t vidSeg;
vir_bytes vidOff;	/* Video ram is found at vidSeg:vidOff */
unsigned vidSize;	/* 0x2000 for color or 0x0800 for mono */
unsigned vidMask;	/* 0x1FFF for color or 0x07FF for mono */
unsigned blankColor = BLANK_COLOR;	/* Display code for blank */

/* Private variables used by the console driver. */
static int vidPort;				/* I/O port for accessing 6845 */
static bool wrap;				/* Hardware can wrap? */
static bool softScroll;			/* true = software scrolling, false = hardware */
static unsigned fontLines;		/* Font lines per character */
static unsigned screenWidth;	/* # characters on a line */
static unsigned screenLines;	/* # lines on the screen */
static unsigned screenSize;		/* # characters on the screen */

typedef struct {
	TTY *c_tty;			/* Associated TTY struct */
	int c_column;		/* Current column number (0-origin) */
	int c_row;			/* Current row (0 at top of screen) */
	int c_rwords;		/* Number of WORDS (not bytes) in outqueue */
	unsigned c_start;	/* Start of video memory of this console */
	unsigned c_limit;	/* Limit of this console's video memroy */
	unsigned c_origin;	/* Location in RAM where 6845 base points */
	unsigned c_cursor;	/* Current position of cursor in video RAM */
	unsigned c_attr;	/* Character attribute */
	unsigned c_blank;	/* Blank attribute */
	char c_esc_state;	/* 0=normal, 1=ESC, 2=ESC[ */
	u16_t c_ram_queue[CONS_RAM_WORDS];	/* Buffer for video RAM */
} Console;

static int numConsoles = 1;		/* Actual number of consoles */
static Console consoleTable[NR_CONS];
static Console *currConsole;	/* Currently visible */

/* Color if using a color controller. */
#define isColor	(vidPort == PORT_C_6845)

static void set6845(int reg, unsigned val) {
/* Set a register pair inside the 6845.
 * Registers 12-13 tell the 6845 where in video ram to start
 * Registers 14-15 tell the 6845 where to put the cursor
 */
	PvBytePair out[4];
	pvSet(out[0], vidPort + REG_INDEX, reg);	/* Set index register */
	pvSet(out[1], vidPort + REG_DATA, (val >> 8) & BYTE);	/* High byte */
	pvSet(out[2], vidPort + REG_INDEX, reg + 1);	/* Again */
	pvSet(out[3], vidPort + REG_DATA, val & BYTE);	/* Low byte */
	sysVecOutb(out, 4);
}

static void flush(register Console *console) {
/* Send characters buffered in 'ramqueue' to screen memory, check the new
 * cursor position, compute the new hardware cursor position and set it.
 */
	unsigned cursor;
	TTY *tp = console->c_tty;

	/* Have the characters in 'ramqueue' transferred to the screen. */
	if (console->c_rwords > 0) {
		mem2VidCopy(console->c_ram_queue, console->c_cursor, console->c_rwords);
		console->c_rwords = 0;
		
		/* TTY likes to know the current column and if echoing messed up. */
		tp->tty_position = console->c_column;
		tp->tty_reprint = true;
	}

	/* Check and update the cursor position. */
	if (console->c_column < 0)
	  console->c_column = 0;
	else if (console->c_column > screenWidth)
	  console->c_column = screenWidth;

	if (console->c_row < 0)
	  console->c_row = 0;
	else if (console->c_row >= screenLines)
	  console->c_row = screenLines - 1;

	cursor = console->c_origin + console->c_row * screenWidth + console->c_column;
	if (cursor != console->c_cursor) {
		if (console == currConsole)
		  set6845(REG_CURSOR, cursor);
		console->c_cursor = cursor;
	}
}

/* Sound is disabled, see ".bochsrc" */
static void beep() {
//TODO
}

static void parseEscape(register Console *console, char c) {
//TODO
}

static void scrollScreen(
	register Console *console,
	int dir		/* SCROLL_UP or SCROLL_DOWN */
) {
	unsigned newLine, newOrigin, chars;

	flush(console);
	chars = screenSize - screenWidth;	/* One screen minus one line */

	/* Scrolling the screen is a real nuisance due to the various incompatible
	 * video cards. This driver supports software scrolling,
	 * hardware scrolling (mono and CGA cards) and hardware scrolling without
	 * wrapping (EGA cards). In the latter case we must make sure that
	 *		c_start <= c_origin && c_origin + screenSize <= c_limit
	 * holds, because EGA doesn't wrap around the end of video memory.
	 */
	if (dir == SCROLL_UP) {
		/* Scroll one line up in 3 ways: soft, avoid wrap, use origin. */
		if (softScroll) {
			vid2VidCopy(console->c_start + screenWidth, console->c_start, chars);
		} else if (!wrap && 
				console->c_origin + screenSize + screenWidth >= console->c_limit) {
			/* Hardware scrolling, no wrap, but c_limit will be exceeeded.
			 * So need copying 'chars' to c_start, then set c_origin to c_start.
			 */
			vid2VidCopy(console->c_origin + screenWidth, console->c_start, chars);
			console->c_origin = console->c_start;
		} else {
			/* Hardware scrolling,
			 * 1. wrap or
			 * 2. not wrap and c_limit will not be excceeded.
			 */
			console->c_origin = (console->c_origin + screenWidth) & vidMask;
		}
		newLine = (console->c_origin + chars) & vidMask;
	} else {
		/* Scroll one line down in 3 ways: soft, avoid wrap, use origin. */
		if (softScroll) {
			vid2VidCopy(console->c_start, console->c_start + screenWidth, chars);
		} else if (!wrap && console->c_origin < console->c_start + screenWidth) {
			newOrigin = console->c_limit - screenSize;
			vid2VidCopy(console->c_origin, newOrigin + screenWidth, chars);
			console->c_origin = newOrigin;
		} else {
			console->c_origin = (console->c_origin - screenWidth) & vidMask;
		}
		newLine = console->c_origin;
	}
	/* Blank the new line at top or bottom. */
	blankColor = console->c_blank;
	mem2VidCopy(BLANK_MEM, newLine, screenWidth);

	/* Set the new video origin. */
	if (console == currConsole)
	  set6845(REG_VID_ORG, console->c_origin);

	flush(console);
}

static void outChar(register Console *console, int c) {
/* Output a character on the console. Check for escape sequences first. */
	if (console->c_esc_state > 0) {
		parseEscape(console, c);
		return;
	}

	switch (c) {
		case 000:		/* Null is typically used for padding */
			return;		/* Better not do anything */
		case 007:		/* Ring the bell */
			flush(console);	/* Print any chars queued for output */
			beep();
			return;
		case '\b':		/* Backspace */
			if (--console->c_column < 0) {
				if (--console->c_row >= 0)
				  console->c_column += screenWidth;
			}
			flush(console);
			return;
		case '\n':		/* Line feed */
			if ((console->c_tty->tty_termios.c_oflag & (OPOST | ONLCR)) == (OPOST | ONLCR))
			  console->c_column = 0;
			/* Fall through */
		case 013:		/* CTRL-K */
		case 014:		/* CTRL-L */
			if (console->c_row == screenLines - 1) 
			  scrollScreen(console, SCROLL_UP);
			else 
			  ++console->c_row;
			flush(console);
			return;
		case '\r':		/* Carriage return */
			console->c_column = 0;
			flush(console);
			return;
		case '\t':		/* Tab */
			console->c_column = (console->c_column + TAB_SIZE) & ~TAB_MASK;
			if (console->c_column > screenWidth) {
				console->c_column -= screenWidth;
				if (console->c_row == screenLines - 1) 
				  scrollScreen(console, SCROLL_UP);
				else
				  ++console->c_row;
			}
			flush(console);
			return;
		case 033:		/* ESC - start of an escape sequence */
			flush(console);		/* Print an y chars queued for output */
			console->c_esc_state = 1;	/* Mark ESC as seen */
			return;
		default:		/* Printable chars are stored in ramqueue */
			if (console->c_column >= screenWidth) {
				if (!LINE_WRAP)
				  return;
				if (console->c_row == screenLines - 1) 
				  scrollScreen(console, SCROLL_UP);
				else
				  ++console->c_row;
				console->c_column = 0;
				flush(console);
			}
			if (console->c_rwords == bufLen(console->c_ram_queue))
			  flush(console);
			console->c_ram_queue[console->c_rwords++] = console->c_attr | (c & BYTE);
			++console->c_column;
			return;
	}
}

static void consoleEcho(register TTY *tp, int c) {
/* Echo keyboard input (print & flush). */
	Console *console = tp->tty_priv;
	
	outChar(console, c);
	flush(console);
}

static int consoleWrite(register TTY *tp, int try) {
/* Copy as much data as possible to the output queue, then start I/O. On
 * memory-mapped terminals, such as the IBM console, the I/O will also be
 * finished, and the cuonts updated. Keep repeating until all I/O done.
 */
	int count;
	int result;
	register char *tb;
	char buf[64];
	Console *console = tp->tty_priv;

	if (try)
	  return 1;		/* We can always write to console. */

	/* Check quickly for nothing to do, so this can be called oftern without
	 * unmodular tests elsewhere.
	 */
	if ((count = tp->tty_out_left) == 0 || tp->tty_inhibited)
	  return 0;

	/* Copy the user bytes to buf[] for decent addressing. Loop over the
	 * copies, since the user buffer may be much larger than buf[].
	 */
	do {
		if (count > sizeof(buf))
		  count = sizeof(buf);
		if ((result = sysVirCopy(tp->tty_out_proc, D, tp->tty_out_vir,
				SELF, D, (vir_bytes) buf, (vir_bytes) count)) != OK) 
		  break;
		tb = buf;

		/* Update terminal data structure. */
		tp->tty_out_vir += count;
		tp->tty_out_cum += count;
		tp->tty_out_left -= count;

		/* Output each byte of the copy to the screen. Avoid calling
		 * outChar() for the "easy" characters, put them into the buffer
		 * directly.
		 */
		do {
			if ((unsigned) *tb < ' ' || console->c_esc_state > 0 ||
					console->c_column >= screenWidth ||
					console->c_rwords >= bufLen(console->c_ram_queue)) {
				outChar(console, *tb++);
			} else {
				console->c_ram_queue[console->c_rwords++] = 
						console->c_attr | (*tb++ & BYTE);
				++console->c_column;
			}
		} while (--count != 0);
	} while ((count = tp->tty_out_left) != 0 && !tp->tty_inhibited);

	flush(console);		/* Transfer anything buffered to the screen. */

	/* Reply to the writer if all output is finished or if an error occured. */
	if (tp->tty_out_left == 0 || result != OK) {
		/* REVIVE is not possible. I/O on memory mapped consoles finishes. */
		ttyReply(tp->tty_out_rep_code, tp->tty_out_caller, 
					tp->tty_out_proc, tp->tty_out_cum);
		tp->tty_out_cum = 0;
	}
	return 0;
}

static int consoleIoctl(register TTY *tp, int try) {
//TODO
return 0;
}

void screenInit(TTY *tp) {
/* Initialize the screen driver. */
	Console *console;
	phys_bytes vidBase;
	u16_t biosColumns, biosCrtBase, biosFontLines;
	u8_t biosRows;
	int line;
	static bool vduInited = false;
	unsigned pageSize;

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
	if (!vduInited) {
		/* How about error checking? What to do on failure??? */
		sysVirCopy(SELF, BIOS_SEG, (vir_bytes) VDU_SCREEN_COLS_ADDR,
				SELF, D, (vir_bytes) &biosColumns, VDU_SCREEN_COLS_SIZE);
		sysVirCopy(SELF, BIOS_SEG, (vir_bytes) VDU_CRT_BASE_ADDR,
				SELF, D, (vir_bytes) &biosCrtBase, VDU_CRT_BASE_SIZE);
		sysVirCopy(SELF, BIOS_SEG, (vir_bytes) VDU_SCREEN_ROWS_ADDR,
				SELF, D, (vir_bytes) &biosRows, VDU_SCREEN_ROWS_SIZE);
		sysVirCopy(SELF, BIOS_SEG, (vir_bytes) VDU_FONTLINES_ADDR,
				SELF, D, (vir_bytes) &biosFontLines, VDU_FONTLINES_SIZE);
		vduInited = true;

		vidPort = biosCrtBase;
		screenWidth = biosColumns;
		fontLines = biosFontLines;
		screenLines = machine.vdu_ega ? biosRows + 1 : 25;

		if (isColor) {
			vidBase = COLOR_BASE;
			vidSize = COLOR_SIZE;
		} else {
			vidBase = MONO_BASE;
			vidSize = MONO_SIZE;
		}
		if (machine.vdu_ega)
		  vidSize = EGA_SIZE;

		wrap = ! machine.vdu_ega;

		sysSegCtl(&vidSegIdx, &vidSeg, &vidOff, vidBase, vidSize);
		
		vidSize >>= 1;	/* Word count */
		vidMask = vidSize - 1;

		/* Size of the screen (number of displayed characters.) */
		screenSize = screenLines * screenWidth;

		/* There can be as many consoles as video memory allows. */
		numConsoles = vidSize / screenSize;
		if (numConsoles > NR_CONS)
		  numConsoles = NR_CONS;
		if (numConsoles > 1)
		  wrap = false;
		pageSize = vidSize / numConsoles;
	}

	console->c_start = line * pageSize;
	console->c_limit = console->c_start + pageSize;
	console->c_cursor = console->c_origin = console->c_start;
	console->c_attr = console->c_blank = BLANK_COLOR;

	if (line != 0) {
		/* Clear the non-console vtys. */
		blankColor = BLANK_COLOR;
		mem2VidCopy(BLANK_MEM, console->c_start, screenSize);
	} else {
		/* Set the cursor of the console vty at the bottom. 
		 * c_cursor is updated automatically later.
		 */
		scrollScreen(console, SCROLL_UP);
		console->c_row = screenLines - 1;
		console->c_column = 0;
	}
	selectConsole(0);
	consoleIoctl(tp, 0);
}

void selectConsole(int consoleLine) {
/* Set the current console to console number 'consoleLine'. */
	if (consoleLine < 0 || consoleLine >= numConsoles)
	  return;

	currConsIdx = consoleLine;
	currConsole = &consoleTable[consoleLine];
	set6845(REG_VID_ORG, currConsole->c_origin);
	set6845(REG_CURSOR, currConsole->c_cursor);
}

static void putk(int c) {
/* This procedure is used by the version of printf() that is linked with
 * the TTY driver. The one in the library sends a message to FS, which is
 * not what is needed for printing within the TTY. This version just queues
 * the character and starts the output.
 */
	if (c != 0) {
		if (c == '\n')
		  putk('\r');
		outChar(&consoleTable[0], c);
	} else {
		flush(&consoleTable[0]);
	}
}

void kputc(int c) {
	putk(c);
}

void doNewKernelMsgs(Message *msg) {
/* Notification for a new kernel message. */
	KernelMsgs kMsgs;
	static int prevNext = 0;	/* Previous next seen */
	int bytes, r;

	/* Try to get a fresh copy of the buffer with kernel messages. */
	sysGetKernelMsgs(&kMsgs);

	/* Print only the new part. Determine how many new bytes there are with 
	 * help of the current and previous 'next' index. Note that the kernel
	 * buffer is circular. This works fine if less then KMESS_BUF_SIZE bytes
	 * is new data; else we miss % KMESS_BUF_SIZE here.
	 * Check for size being positive, the buffer might as well be emptied!
	 */
	if (kMsgs.km_size > 0) {
		bytes = (kMsgs.km_next + KMESS_BUF_SIZE - prevNext) % KMESS_BUF_SIZE;
		r = prevNext;	/* Start at previous old */
		while (bytes > 0) {
			putk(kMsgs.km_buf[r % KMESS_BUF_SIZE]);
			--bytes;
			++r;
		}
		putk(0);	/* Terminate to flush output */
	}

	/* Almost done, store 'next' so that we can determine what part of the
	 * kernel messages buffer to print next time a notification arrives.
	 */
	prevNext = kMsgs.km_next;
}

static void consoleOriginAt0() {
/* Scroll video memory back to put the origin at 0. */
	int consoleLine;
	Console *console;
	unsigned n;

	for (consoleLine = 0; consoleLine < numConsoles; ++consoleLine) {
		console = &consoleTable[consoleLine];
		while (console->c_origin > console->c_start) {
			n = MIN(vidSize - screenSize,	/* Amount of unused memory */
					console->c_origin - console->c_start);
			vid2VidCopy(console->c_origin, console->c_origin - n, screenSize);
			console->c_origin -= n;
		}
		flush(console);
	}
	selectConsole(currConsIdx);
}

void consoleStop() {
/* Prepare for halt or reboot. */
	consoleOriginAt0();
	softScroll = true;
	selectConsole(0);
	consoleTable[0].c_attr = consoleTable[0].c_blank = BLANK_COLOR;
}

void doDiagnostics(Message *msg) {
/* Print a string for a server. */
	char c;
	int count;
	int result = OK;
	vir_bytes src;
	int pNum = msg->DIAG_PROC_NR;
	if (pNum == SELF) 
	  pNum = msg->m_source;

	src = (vir_bytes) msg->DIAG_PRINT_BUF;
	for (count = msg->DIAG_BUF_COUNT; count > 0; --count) {
		if (sysVirCopy(pNum, D, src++, SELF, D, (vir_bytes) &c, 1) != OK) {
			result = EFAULT;
			break;
		}
		putk(c);
	}
	putk(0);	/* Always terminate, even with EFAULT */
	msg->m_type = result;
	send(msg->m_source, msg);
}

void toggleScroll() {
/* Toggle between hardware and software scroll. */
	consoleOriginAt0();
	softScroll = ! softScroll;
	printf("%sware scrolling enabled.\n", softScroll ? "Soft" : "Hard");
}







