#include "../drivers.h"
#include "sys/time.h"
#include "sys/select.h"
#include "termios.h"
#include "signal.h"
#include "unistd.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "minix/keymap.h"
#include "tty.h"
#include "keymaps/us-std.src"
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"
#include "../../kernel/proc.h"

int irqHookId = -1;

/* Standard and AT keyboard.  (PS/2 MCA implies AT throughout.) */
#define REG_KB_DATA			0x60	/* I/O port for keyboard data */

/* AT keyboard. */
#define REG_KB_COMMAND		0x64	/* I/O port for commands on AT */
#define REG_KB_STATUS		0x64	/* I/O port for status on AT */
#define		KB_ACK			0xFA	/* Keyboard ack response */
#define		KB_OUT_FULL		0x01	/* Status bit set when keypress char pending */
#define		KB_IN_FULL		0x02	/* Status bit set when not ready to receive */
#define CMD_LED_CODE		0xED	/* Command to keyboard to set LEDs */
#define MAX_KB_ACK_RETRIES	0x1000	/* Max #times to wait for keyboard ack */
#define MAX_KB_BUSY_RETRIES	0x1000	/* Max #times to loop while keyboard busy */
#define KBIT				0x80	/* Bit used to ack characters to keyboard */

#define ESC_SCAN_CODE		0x01	/* Reboot key when panicking */
#define SLASH_SCAN_CODE		0x35	/* To recognize numeric slash */
#define R_SHIFT_SCAN_CODE	0x36	/* To distinguish left and right shift */
#define HOME_SCAN_CODE		0x47	/* First key on the numeric keypad */
#define INS_SCAN_CODE		0x52	/* INSERT scan code(82) for use in CTRL-ALT-INS reboot */
#define DEL_SCAN_CODE		0x53	/* DELETE scan code(83) for use in CTRL-ALT-DEL reboot */

#define CONSOLE				0		/* Line number for console */
#define KB_IN_BYTES			32		/* Size of keyboard input buffer */
static char inBuf[KB_IN_BYTES];		/* Input buffer */
static char *inHead = inBuf;		/* Next free sopt in input buffer */
static char *inTail = inBuf;		/* Scan code to return to TTY */
static int inCount;					/* # codes in buffer */

static int esc;				/* Escape scan code detected? */
static int altLeft;			/* Left alt key state */
static int altRight;		/* Right alt key state */
static int alt;				/* Either alt key */
static int ctrlLeft;		/* Left control key state */
static int ctrlRight;		/* Right control key state */
static int ctrl;			/* Either control key */
static int shiftLeft;		/* Left shift key state */
static int shiftRight;		/* Right shift key state */
static int shift;			/* Either shift key */
static int numDown;			/* Num lock key depressed */
static int capsDown;		/* Caps lock key depressed */
static int scrollDown;		/* Scroll lock key depressed */
static int locks[NR_CONS];	/* Per console lock keys state */

/* Lock key active bits. Chosen to be equal to the keyboard LED bits. */
#define SCROLL_LOCK		0x01
#define NUM_LOCK		0x02
#define CAPS_LOCK		0x04

static char numPadMap[] = {
	'H', 'Y', 'A', 'B', 'D', 'C', 'V', 'U', 'G', 'S', 'T', '@'
};

/* Variables and definition for observed function keys. */
typedef struct {
	int pNum;
	int events;
} Observer;
static Observer fKeyObs[12];	/* Observers for F1-F12 */
static Observer sfKeyObs[12];	/* Observers for SHIFT F1-F12 */


static int kbWait() {
/* Wait until the controller is ready; return zero if this times out. */
	int retries, status, temp;

	status = 0;
	retries = MAX_KB_BUSY_RETRIES + 1;
	do {
		sysInb(REG_KB_STATUS, &status);
		if (status & KB_OUT_FULL)
		  sysInb(REG_KB_DATA, &temp);		/* Discard value */
		if (!(status & (KB_IN_FULL | KB_OUT_FULL)))
		  break;
	} while (--retries != 0);	/* Continue unless timeout */

	return retries;		/* Zero on timeout, positive if ready */
}

static int kbAck() {
/* Wait until keyboard acknowledges last command; return zero if this times out. */
	int retries;
	u8_t val;

	retries = MAX_KB_ACK_RETRIES + 1; /* Wait until not busy */
	do {
		sysInb(REG_KB_DATA, &val);
		if (val == KB_ACK)
		  break;		/* Wait for ack */
	} while (--retries != 0);	/* Continue unless timeout */

	return retries;	/* Nonzero if ack received */
}

static void setLeds() {
/* set the LEDs on the caps, num, and scroll lock keys */
	int s;
	if (!machine.pc_at) 
	  return;	/* PC/XT doesn't have LEDs */

	kbWait();	/* Wait for buffer empty */
	/* Prepare keyboard to accept LED values. */
	if ((s = sysOutb(REG_KB_DATA, CMD_LED_CODE)) != OK) 
	  printf("Warning, sysOutb couldn't prepare for LED values: %d\n", s);
	
	kbAck();	/* Wait for ack response */

	kbWait();	/* wait for buffer empty */
	/* Give keyboard LED values */
	if ((s = sysOutb(REG_KB_DATA, locks[currConsIdx])) != OK)
	  printf("Warning, sysOutb couldn't give LED values: %d\n", s);
	
	kbAck();	/* Wait for ack response */
}

static unsigned mapKey(int scanCode) {
/* Map a scan code to an ASCII code. */

	int caps, lock, column;
	u16_t *keyRow;

	if (scanCode == SLASH_SCAN_CODE && esc)
	  return '/';	/* Don't map numeric slash */
	
	keyRow = &keyMap[scanCode * MAP_COLS];
	
	caps = shift;
	lock = locks[currConsIdx];
	if ((lock & NUM_LOCK) && HOME_SCAN_CODE <= scanCode && scanCode <= DEL_SCAN_CODE)
	  caps = !caps;
	if ((lock & CAPS_LOCK) && (keyRow[0] & HAS_CAPS))
	  caps = !caps;

	if (alt) {
		column = 2;	
		if (ctrl || altRight)
		  column = 3;	/* Ctrl + Alt = AltGr */
		if (caps)
		  column = 4;
	} else {
		column = 0;
		if (caps)
		  column = 1;
		if (ctrl)
		  column = 5;
	}
	return keyRow[column] & ~HAS_CAPS;
}

static int isFuncKey(int scanCode) {
/* This procedure traps function keys for debugging purposes. Observers of
 * function keys are kept in a global array. If a subject (a key) is pressed
 * the observer is notified of the event. Initialization of the arrays is done
 * in kbInit, where NONE is set to indicate there is no interest in the key.
 * Returns FALSE on a key release or if the key is not observable.
 */
	int key;
	int pNum;

	/* Ignore key releases. If this is a key press, get full key code. */
	if (scanCode & KEY_RELEASE)
	  return false;		/* Key release */

	key = mapKey(scanCode);	/* Include modifiers */
	
	/* Key pressed, now see if there is an observer for the pressed key.
	 *           F1-F12   observers are in fKeyObs array.
	 *    SHIFT  F1-F12   observers are in sfKeyObs array.
	 *    CTRL   F1-F12   reserved (see kbRead)
	 *    ALT    F1-F12   reserved (see kbRead)
	 * Other combinations are not in use. Note that Alt+Shift+F1-F12 is yet
	 * defined in "minix/keymap.h", and thus is easy for future extensions.
	 */
	if (F1 <= key && key <= F12) {	/* F1-F12 */
		pNum = fKeyObs[key - F1].pNum;
		++fKeyObs[key - F1].events;
	} else if (SF1 <= key && key <= SF12) {	/* Shift F1-F12 */
		pNum = sfKeyObs[key - SF1].pNum;
		++sfKeyObs[key - SF1].events;
	} else {
		return false;	/* Not observable */
	}

	/* See if an observer is registered and send it a message. */
	if (pNum != NONE) {
		notify(pNum);
	}

	return true;
}

static unsigned makeBreak(int scanCode) {
/* This routine can handle keyboards that interrupt only on key depression,
 * as well as keyboards that interrupt on key depression and key release.
 * For efficiency, the interrupt routine filters out most key releases.
 */
	int ch, make, escape;
	static int countCAD = 0;

	/* Check for CTRL-ALT-DEL, and if found, halt the computer. This would
	 * be better done in keyboard() in case TTY is hung, except control and
	 * alt are set in the high level code.
	 */
	if (ctrl && alt && (scanCode == DEL_SCAN_CODE || scanCode == INS_SCAN_CODE)) {
		if (++countCAD == 3)
		  sysAbort(RBT_HALT);
		sysKill(INIT_PROC_NR, SIGABRT);
		return -1;
	}

	/* High-order bit set on key release. */
	make = (scanCode & KEY_RELEASE) == 0;	/* True if pressed */

	ch = mapKey(scanCode &= ASCII_MASK);	/* Map to ASCII */

	escape = esc;		/* Key is escaped? (true if added since the XT) */
	esc = 0;

	switch (ch) {
		case CTRL:		/* Left or right control key */
			*(escape ? &ctrlRight : &ctrlLeft) = make;
			ctrl = ctrlLeft | ctrlRight;
			break;
		case SHIFT:		/* Left or right shift key */
			*(scanCode == R_SHIFT_SCAN_CODE ? &shiftRight : &shiftLeft) = make;
			shift = shiftLeft | shiftRight;
			break;
		case ALT:		/* Left or right alt key */
			*(escape ? &altRight : &altLeft) = make;
			alt = altLeft | altRight;
			break;
		case CALOCK:	/* Caps lock - toggle on 0 -> 1 transition */
			/* Init: capsDown = 0, caps = false
			 *
			 * T1: key down => make = 1, 
			 *     capsDown < make => toggle caps to true, LED turns on, capsDown = 1 
			 * T2: key up => make = 0,
			 *     capsDown > make => caps retains true, capsDown = 0
			 *
			 * T3: key down => make = 1,
			 *     capsDown < make => toggle caps to false, LED turns off, capsDown = 1 
			 * T4: key up => make = 0,
			 *     capsDown > make => caps retains false, capsDown = 0
			 */
			if (capsDown < make) {	/* key down */
				locks[currConsIdx] ^= CAPS_LOCK;
				setLeds();
			}
			capsDown = make;	/* key down/up */
			break;
		case NLOCK:		/* Num lock */
			if (numDown	< make) {	
				locks[currConsIdx] ^= NUM_LOCK;
				setLeds();
			}
			numDown = make;
			break;
		case SLOCK:		/* Scroll lock */
			if (scrollDown < make) {
				locks[currConsIdx] ^= SCROLL_LOCK;
				setLeds();
			}
			scrollDown = make;
			break;
		case EXT_KEY:	/* Escape keycode */
			/* When right alt/ctrl is pressed, it will produce two scan codes:
			 * 0xE0 and (alt/ctrl).
			 *
			 * When processing the scan code 0xE0,
			 * 1. scanCode &= ASCII_MASK => 0x60 = 96
			 * 2. mapKey(96) => EXT_KEY
			 */
			esc = 1;	/* Next key is escaped */
			return -1;
		default:		/* A normal key */
			if (make)
			  return ch;
	}

	/* Key release, or a shift type key. */
	return -1;
}

static void showKeyMappings() {
//TODO
}

static int kbRead(TTY *tp, int try) {
/* Process characters from the circular keyboard buffer. */
	char buf[3];
	int scanCode;
	unsigned ch;

	tp = &ttyTable[currConsIdx];

	if (try)
	  return inCount > 0 ? 1 : 0;

	while (inCount > 0) {
		scanCode = *inTail++;	/* Take one key scan code */
		if (inTail == inBuf + KB_IN_BYTES) 
		  inTail = inBuf;
		--inCount;

		/* Function keys are being used for debug dumps. */
		if (isFuncKey(scanCode))
		  continue;

		/* Perform make/break processing. */
		ch = makeBreak(scanCode);
		
		if (ch <= 0xFF) {
			/* A normal character. */
			buf[0] = ch;
			inProcess(tp, buf, 1);
		} else if (HOME <= ch && ch <= INSRT) {
			/* An ASCII escape sequence generated by the numeric pad. */
			buf[0] = ESC;
			buf[1] = '[';
			buf[2] = numPadMap[ch - HOME];
			inProcess(tp, buf, 3);
		} else if (ch == ALEFT) {
			/* Choose lower numbered console as current console. */
			selectConsole(currConsIdx - 1);
			setLeds();
		} else if (ch == ARIGHT) {
			/* Choose higher numbered console as current console. */
			selectConsole(currConsIdx + 1);
			setLeds();
		} else if (AF1 <= ch && ch <= AF12) {
			selectConsole(ch - AF1);
			setLeds();
		} else if (CF1 <= ch && ch <= CF12) {
			switch (ch) {
				case CF1:
					showKeyMappings();
					break;
				case CF3:
					toggleScroll();		/* Hardware <-> software */
					break;
				case CF7:
					signalChar(&ttyTable[CONSOLE], SIGQUIT);
					break;
				case CF8:
					signalChar(&ttyTable[CONSOLE], SIGINT);
					break;
				case CF9:
					signalChar(&ttyTable[CONSOLE], SIGKILL);
					break;
			}
		}
	}
	return 1;
}

void kbInit(TTY *tp) {
/* Initialize the keyboard driver. */
	tp->tty_dev_read = kbRead;	/* Input function */
}

static int scanKeyboard() {
/* Fetch the character from the keyboard hardware and acknowledge it. */
	PvBytePair byteIn[2], byteOut[2];

	byteIn[0].port = REG_KB_DATA;	/* Get the scan code for the key struck */
	byteIn[1].port = PORT_B;		/* Stroke the keyboard to ack the char */
	sysVecInb(byteIn, 2);			/* Request actual input */

	pvSet(byteOut[0], PORT_B, byteIn[1].value | KBIT);	/* Stroke bit high */
	pvSet(byteOut[1], PORT_B, byteIn[1].value);		/* Then stroke low */
	sysVecOutb(byteOut, 2);			/* Request actual output */

	return byteIn[0].value;			/* Return scan code */
}

void kbInitOnce() {
	int i;
	
	setLeds();			/* Turn off numlock led */
	scanKeyboard();		/* Discard leftover keystroke */

	/* Clear the function key observers array. Also see funcKey(). */
	for (i = 0; i < 12; ++i) {
		fKeyObs[i].pNum = NONE;		/* F1-F12 observers */		
		fKeyObs[i].events = 0;
		sfKeyObs[i].pNum = NONE;	/* Shift F1-F12 Observers */
		sfKeyObs[i].events = 0;
	}
	if(true)return;//TODO

	/* Set interrupt handler and enable keyboard IRQ. */
	irqHookId = KEYBOARD_IRQ;	/* Id to be returned on interrupt. */
	if ((i = sysIrqSetPolicy(KEYBOARD_IRQ, IRQ_REENABLE, &irqHookId)) != OK)
	  panic("TTY", "Couldn't set keyboard IRQ policy", i);
	if ((i = sysIrqEnable(&irqHookId)) != OK)
	  panic("TTY", "Couldn't enable keyboard IRQs", i);

	kbdIrqSet |= (1 << KEYBOARD_IRQ);
}

void kbdInterrupt() {
/* A keyboard interrupt has occurred. Process it. */
	int scanCode;

	/* Fetch the character from the keyboard hardware and acknowledge it. */
	scanCode = scanKeyboard();

	/* Store the scancode in memory so the task can get at it later. */
	if (inCount < KB_IN_BYTES) {
		*inHead++ = scanCode;
		if (inHead == inBuf + KB_IN_BYTES)
		  inHead = inBuf;
		++inCount;
		ttyTable[currConsIdx].tty_events = 1;
		if (ttyTable[currConsIdx].tty_select_ops && SEL_RD) {
			selectRetry(&ttyTable[currConsIdx]);
		}
	}
}

void doPanicDumps(Message *msg) {
//TODO
}

void doFKeyCtl(Message *msg) {
//TODO
}


