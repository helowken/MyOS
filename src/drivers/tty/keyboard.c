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
#define MAX_KB_BUSY_RETRIES	0x1000	/* Max #times to loop while keyboard busy */

static int locks[NR_CONS];	/* Per console lock keys state */

/* Variables and definition for observed function keys. */
typedef struct {
	int pNum;
	int events;
} Observer;
static Observer fKeyObs[12];	/* Observers for F1-F12 */
static Observer sfKeyObs[12];	/* Observers for SHIFT F1-F12 */


static void kbWait() {
/* Wait until the controller is ready; return zero if this times out. */
	int retries, status, temp;
	int s;

	retries = MAX_KB_BUSY_RETRIES + 1;
	do {
		s = sysInb(REG_KB_STATUS, &status);
		if (status & KB_OUT_FULL)
		  s = sysInb(REG_KB_DATA, &temp);		/* Discard value */
		if (!(status & (KB_IN_FULL | KB_OUT_FULL)))
		  break;
	} while (--retries != 0);	/* Continue unless timeout */

	return retries;		/* Zero on timeout, positive if ready */
}

static void kbAck() {
/* Wait until keyboard acknowledges last command; return zero if this times out. */
	int retries, s;
	u8_t val;

	retries = MAX_KB_BUSY_RETRIES + 1; /* Wait until not busy */
	do {
		s = sysInb(REG_KB_DATA, &val);
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
	if ((s = sysOutb(REG_KB_DATA, locks[currConsule])) != OK)
	  printf("Warning, sysOutb couldn't give LED values: %d\n", s);
	
	kbAck();	/* Wait for ack response */
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

	/* Set interrupt handler and enable keyboard IRQ. */
	irqHookId = KEYBOARD_IRQ;	/* Id to be returned on interrupt. */
	if ((i = sysIrqSetPolicy(KEYBOARD_IRQ, IRQ_REENABLE, &irqHookId)) != OK)
	  panic("TTY", "Couldn't set keyboard IRQ policy", i);
	if ((i = sysIrqEnable(&irqHookId)) != OK)
	  panic("TTY", "Couldn't enable keyboard IRQs", i);

	kbdIrqSet |= (1 << KEYBOARD_IRQ);
}



