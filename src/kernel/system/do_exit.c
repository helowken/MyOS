#include "../system.h"

static void clearProc(register Proc *rc) {
	register Proc *xp;
	register Proc **xpp;
	int i;

	/* Turn off any alarm timers at the clock. */
	resetTimer(&priv(rc)->s_alarm_timer);

	/* Make sure that the exiting process is no longer scheduled. */
	if (rc->p_rts_flags == 0)
	  lockDequeue(rc);

	/* If the process being terminated happens to be queued trying to send a
	 * message (e.g., the process was killed by a signal, rather than it doing
	 * a normal exit), then it must be removed from the message queues.
	 */
	if (rc->p_rts_flags & SENDING) {
		/* Check all proc slots to see if the exiting process is queued. */
		for (xp = BEG_PROC_ADDR; xp < END_PROC_ADDR; ++xp) {
			if (xp->p_sender_head == NIL_PROC)
			  continue;
			/* Make sure that the exiting process is not on the queue. */
			xpp = &xp->p_sender_head;
			while (*xpp != NIL_PROC) {		/* Check entire queue. */
				if (*xpp == rc) {		/* Process is on the queue. */
					*xpp = rc->p_sender_next;	/* Replace by next process. */
					break;		
				}
				xpp = &(*xpp)->p_sender_next;		/* Proceed to next queued. */
			}
		}
	}

	/* Check the table with IRQ hooks to see if hooks should be released. */
	for (i = 0; i < NR_IRQ_HOOKS; ++i) {
		if (irqHooks[i].pNum == procNum(rc)) {
			removeIrqHandler(&irqHooks[i]);		/* Remove interrupt handler. */
			irqHooks[i].pNum = NONE;		/* Mark hook as free. */
		}
	}
	
	/* Now it is safe to release the process table slot. If this is a system
	 * process, also release its privilege structure. Further cleanup is not
	 * needed at this point. All important fields are reinitialized when the 
	 * slots are assigned to another, new process.
	 */
	rc->p_rts_flags = SLOT_FREE;
	if (priv(rc)->s_flags & SYS_PROC)
	  priv(rc)->s_proc_nr = NONE;
}

int doExit(Message *msg) {
/*	Handle sys_exit. A user process has exited or a system process requests
 *	to exit. Only the PM can request other process slots to be cleared.
 *	The routine to clean up a process table slot cancels outstanding timers,
 *	possibly removes the process from the message queues, and resets certain
 *	process table fields to the default values.
 */
	int exitProcNum;

	/* Determine what process exited. User processes are handled here. */
	if (PM_PROC_NR == msg->m_source) {
		exitProcNum = msg->PR_PROC_NR;	/* Get exiting process */
		if (exitProcNum != SELF) {
			if (! isOkProcNum(exitProcNum))
			  return EINVAL;
			clearProc(procAddr(exitProcNum));	/* Exit a user process */
			return OK;		/* Report back to PM */
		}
	} 

	/* The PM or some other system process requested to be exited. */
	clearProc(procAddr(msg->m_source));
	return EDONTREPLY;
}
