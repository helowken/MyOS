#include "pm.h"
#include "signal.h"
#include "stdlib.h"
#include "sys/resource.h"
#include "string.h"
#include "mproc.h"

#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"
#include "../../kernel/proc.h"

static void pmInit() {
/* Initialize the process manager. */
	int s;
	//static BootImage images[NR_BOOT_PROCS];
	//register BootImage *ip;
	static char coreSigs[] = { SIGQUIT, SIGILL, SIGTRAP, SIGABRT,
		SIGFPE, SIGUSR1, SIGSEGV, SIGUSR2 };
	static char ignoreSigs[] = { SIGCHLD };
	register MProc *rmp;
	register char *sigPtr;
		
	/* Initialize process table, including timers. */
	for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
		timerInit(&rmp->mp_timer);
	}

	/* Build the set of signals which cause core dumps, and the set of signals
	 * that are by default ignored.
	 */
	sigemptyset(&coreSigSet);
	for (sigPtr = coreSigs; sigPtr < coreSigs + sizeof(coreSigs); ++sigPtr) {
		sigaddset(&coreSigSet, *sigPtr);
	}
	sigemptyset(&ignoreSigSet);
	for (sigPtr = ignoreSigs; sigPtr < ignoreSigs + sizeof(ignoreSigs); ++sigPtr) {
		sigaddset(&ignoreSigSet, *sigPtr);
	}

	/* Obtain a copy of the boot monitor parameters and the kernel info struct.
	 * Parse the list of free memory chunks. This list is what the boot monitor
	 * reported, but it must be corrected for the kernel and system processes.
	 */
	if ((s = sysGetMonParams(monitorParams, sizeof(monitorParams))) != OK)
	  panic(__FILE__, "Get monitor params failed", s);


}

static void getWork() {
/* Wait for the next message and extract useful information from it. */
	if (receive(ANY, &inMsg) != OK)
	  panic(__FILE__, "PM receive error", NO_NUM);
	
	who = inMsg.m_source;		/* Who sent the message */
	callNum = inMsg.m_type;		/* System call number */

	/* Process slot of caller. Misuse PM's own process slot if the kernel is
	 * calling. This can happen in case of synchronous alarms (CLOCK) or event
	 * like pending kernel signals (SYSTEM).
	 */
	mp = &mprocTable[who < 0 ? PM_PROC_NR : who];
}

int main() {
/* Main routine of the process manager. */

	pmInit();		/* Initialize process manager tables. */

	while (true) {
		getWork();	/* Wait for an PM system call. */
	}

	return OK;
}
