#include "is.h"

#define N_HOOKS	15

typedef struct {
	int key;
	void (*function)(void);
	char *name;
} HookEntry;

static HookEntry hooks[N_HOOKS] = {
	{ F1,   procTabDmp,		"Kernel process table" },
	{ F2,   memmapDmp,		"Process memory maps" },
	{ F3,   imageDmp,		"System image" },
	{ F4,   privilegesDmp,  "Process privileges" },
	{ F5,   monParamsDmp,   "Boot monitor parameters" },
	{ F6,   irqTabDmp,      "IRQ hooks and policies" },
	{ F7,   kernelMsgDmp,   "Kernel messages" },
	/* F8-F9 */
	{ F10,  kernelEnvDmp,   "Kernel parameters" },
	{ F11,  timingDmp,      "Timeing details (if enabled)" },
	{ F12,  scheduleDmp,    "Scheduling queues" },

	{ SF1,  mprocDmp,       "Process manager process table" },
	{ SF2,  sigactionDmp,   "Signals" },
	{ SF3,  fprocDmp,       "File system process table" },
	{ SF4,  devTabDmp,      "Device/Driver mapping" },
	{ SF5,  mappingDmp,     "Print key mappings" }
	/* SF6-SF12 */
};

#define isPressed(m, k)  \
	((F1 <= (k) && (k) <= F12 && bitIsSet((m)->FKEY_FKEYS, ((k) - F1 + 1))) || \
	 (SF1 <= (k) && (k) <= SF12 && bitIsSet((m)->FKEY_SFKEYS, ((k) - SF1 + 1))))


int doFKeyPressed(Message *msg) {
	int s, h;

	/* The notification message does not convey any information, other
	 * than that some function keys have been pressed. Ask TTY for details.
	 */
	msg->m_type = FKEY_CONTROL;
	msg->FKEY_REQUEST = FKEY_EVENTS;
	if ((s = sendRec(TTY_PROC_NR, msg)) != OK)
	  report("IS", "warning, sendRec to TTY failed", s);

	/* Now check which keys were pressed: F1-F12. */
	for (h = 0; h < N_HOOKS; ++h) {
		if (isPressed(msg, hooks[h].key))
		  hooks[h].function();
	}

	/* Inhibit sending a reply message. */
	return EDONTREPLY;
}

static char *keyName(int key) {
	static char name[15];

	if (key >= F1 && key <= F12)
	  sprintf(name, " F%d", key - F1 + 1);
	else if (key >= SF1 && key <= SF12)
	  sprintf(name, "Shift+F%d", key - SF1 + 1);
	else
	  sprintf(name, "?");

	return name;
}

void mappingDmp(void) {
	int h;

	printf("Function key mappings for debug dumps in IS server.\n"
		   "        Key   Descriptiong\n"
		   "-------------------------------------------------------------------------\n");
	for (h = 0; h < N_HOOKS; ++h) {
		printf(" %10s.  %s\n", keyName(hooks[h].key), hooks[h].name);
	}
	printf("\n");
}


