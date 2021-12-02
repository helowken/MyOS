#include "kernel.h"
#include "system.h"
#include "stdlib.h"
#include "unistd.h"
#include "protect.h"

int (*callVec[NR_SYS_CALLS])(Message *msg);

/* Declaration of the call vector that defines the mapping of kernel calls
 * to handler functions. The vector is initialized in initialize() with map(),
 * which makes sure the kernel call numbers are ok. 
 */
#define map(callNum, handler)	callVec[(callNum - KERNEL_CALL)] = (handler)

static void initialize() {
	register Priv *sp;
	int i;

	/* Initialize IRQ handler hooks. Mark all hooks available. */
	for (i = 0; i < NR_IRQ_HOOKS; ++i) {
		irqHooks[i].procNum = NONE;
	}

	/* Initialize all alarm timers for all processes. */
	for (sp = BEG_PRIV_ADDR; sp < END_PRIV_ADDR; ++sp) {
		timerInit(&sp->s_alarm_timer);
	}

	/* Initialize the call vector to a safe default handler. */
	for (i = 0; i < NR_SYS_CALLS; ++i) {
		callVec[i] = doUnused;	
	}

	/* Process management. */
	map(SYS_FORK, doFork);
	map(SYS_EXEC, doExec);
	map(SYS_EXIT, doExit);
	map(SYS_NICE, doNice);
	map(SYS_PRIVCTL, doPrivCtl);
	map(SYS_TRACE, doTrace);

	/* Signal handling. */
	map(SYS_KILL, doKill);
	map(SYS_GETKSIG, doGetKSig);
	map(SYS_ENDKSIG, doEndKSig);
	map(SYS_SIGSEND, doSigSend);
	map(SYS_SIGRETURN, doSigReturn);

	/* Device I/O. */
	map(SYS_IRQCTL, doIrqCtl);
	map(SYS_DEVIO, doDevIO);
	map(SYS_SDEVIO, doStrDevIO);
	map(SYS_VDEVIO, doVecDevIO);
	map(SYS_INT86, doInt86);

	/* Memory management. */
	map(SYS_NEWMAP, doNewMap);
	map(SYS_SEGCTL, doSegCtl);
	map(SYS_MEMSET, doMemset);
	
	/* Copying. */
	map(SYS_UMAP, doUmap);
	map(SYS_VIRCOPY, doVirCopy);
	map(SYS_PHYSCOPY, doPhysCopy);
	map(SYS_VIRVCOPY, doVirVecCopy);
	map(SYS_PHYSVCOPY, doPhysVecCopy);

	/* Clock functionality. */
	map(SYS_TIMES, doTimes);
	map(SYS_SETALARM, doSetAlarm);

	/* System control. */
	map(SYS_ABORT, doAbort);
	map(SYS_GETINFO, doGetInfo);
}

/* Main entry point of sysTask. Get the message and dispatch on type. */
void sysTask() {
	static Message msg;
	register int result;
	register Proc *caller;
	unsigned int callNum;
	int s;

	/* Initialize the system task. */
	initialize();

	while (true) {
		/* Get work. Block and wait until a request message arrives. */
		receive(ANY, &msg);
		callNum = (unsigned) msg.m_type - KERNEL_CALL;
		caller = procAddr(msg.m_source);

		/* See if the caller made a valid request and try to handle it. */
		if (! (priv(caller)->s_call_mask & (1 << callNum))) {
			kprintf("SYSTEM: request %d from %d denied. \n", callNum, msg.m_source);
			result = ECALLDENIED;		/* Illegal message type. */
		} else if (callNum >= NR_SYS_CALLS) {	/* Check call number. */
			kprintf("SYSTEM: illegal request %d from %d.\n", callNum, msg.m_source);
			result = EBADREQUEST;		/* Illegal message type. */
		} else {
			result = (*callVec[callNum])(&msg);	/* Handle the kernel call. */
		}

		/* Send a reply, unless inhibited by a handler function. Use the kernel
		 * function lockSend() to prevent a system call trap. The destination
		 * is known to be blocked waiting for a message.
		 */
		if (result != EDONTREPLY) {
			msg.m_type = result;		/* Report status of call. */
			if (OK != (s = lockSend(msg.m_source, &msg))) {
				kprintf("SYSTEM, reply to %d failed: %d\n", msg.m_source, s);
			}
		}
	}
}

int getPriv(register Proc *rp, int procType) {
	register Priv *sp;

	if (procType == SYS_PROC) {
		/* Find a new slot */
		for (sp = BEG_PRIV_ADDR; sp < END_PRIV_ADDR; ++sp) {
			if (sp->s_proc_nr == NONE && sp->s_id != USER_PRIV_ID)
			  break;
		}
		if (sp->s_proc_nr != NONE)
		  return ENOSPC;
		rp->p_priv = sp;				/* Assign new slot */
		sp->s_proc_nr = procNum(rp);	/* Set association */
		sp->s_flags = SYS_PROC;			/* Mark as privileged */
	} else {
		rp->p_priv = &privTable[USER_PRIV_ID];	/* Use shared slot */
		rp->p_priv->s_proc_nr = INIT_PROC_NR;	/* Set association */
		rp->p_priv->s_flags = 0;
	}
	return OK;
}





