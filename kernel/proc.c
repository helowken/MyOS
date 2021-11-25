#include "minix/com.h"
#include "kernel.h"
#include "proc.h"

/* This function determines the scheduling policy. It is called whenever a
 * process must be added to one of the scheduling queues to decide where to
 * insert it. As a side-effect the process' priority may be update.
 */
static void schedule(Proc *rp, int *queue, int *front) {
	static Proc *prevProc = NIL_PROC;		/* Previous without time */
	int timeLeft = rp->p_ticks_left > 0;	/* Quantum fully consumed */
	int penalty = 0;		/* change in priority */

	if (!timeLeft) {	/* Quantum consumed ? */
		rp->p_ticks_left = rp->p_quantum_size;	/* Give new quantum */
		if (prevProc == rp) 
		  ++penalty;		/* Catch infinite loops */
		else 
		  --penalty;		/* Give slow way back */
		prevProc = rp;		/* Store ptr for next check */
	}

	/* Determine the new priority of this process. The bounds are determined
	 * by IDLE's queue and the maximum priority of this process. Kernel task
	 * and the idle process are never changed in priority.
	 */
	if (penalty != 0 && !isKernelProc(rp)) {
		rp->p_priority += penalty;		/* Update with penalty */
		if (rp->p_priority < rp->p_max_priority)	/* Check upper bound */
		  rp->p_priority = rp->p_max_priority;
		else if (rp->p_priority >= IDLE_Q)		/* Check lower bound */
		  rp->p_priority = IDLE_Q - 1;
	}

	/* If there is time left, the process is added to the front of its queue,
	 * so that it can immediately run. The queue to use simply is always the 
	 * process' current priority.
	 */
	*queue = rp->p_priority;
	*front = timeLeft;
}

/* Decide who to run now. A new process is selected by setting 'nextProc'.
 * When a billable process is selected, record it in 'billProc', so that the
 * clock task can tell who to bill for system time.
 */
static void pickProc() {
	register Proc *rp;	/* Process to run */
	int queue;			/* Iterate over queues */

	/* Check each of the scheduling queues for ready processes. The number of
	 * queues is defined in proc.h, and priorities are set in the image table.
	 * The lowest queue contains IDLE, which is always ready.
	 */
	for (queue = 0; queue < NR_SCHED_QUEUES; ++queue) {
		if ((rp = readyProcHead[queue]) != NIL_PROC) {
			nextProc = rp;
			if (priv(rp)->s_flags & BILLABLE)
			  billProc = rp;
			return;
		}
	}
}

static void enqueue(register Proc *rp) {
	int queue;	/* Scheduling queue to use */
	int front;	/* Add to front or back */

	schedule(rp, &queue, &front);

	/* Now add the process to the queue. */
	if (readyProcHead[queue] == NIL_PROC) {		/* Add to empty queue */
		readyProcHead[queue] = readyProcTail[queue] = rp;	/* Create a new queue */
		rp->p_next_ready = NIL_PROC;			/* Mark new end */
	} else if (front) {			/* Add to head of queue */
		rp->p_next_ready = readyProcHead[queue];	/* Chain head of queue */	
		readyProcHead[queue] = rp;		/* Set new queue head */
	} else {			/* add to tail of queue */
		readyProcTail[queue]->p_next_ready = rp;	/* Chain tail of queue */
		readyProcTail[queue] = rp;		/* Set new queue tail */
		rp->p_next_ready = NIL_PROC;	/* Mark new end */
	}

	/* Now select the next process to run. */
	pickProc();
}

static void dequeue(register Proc *rp) {
	register int queue = rp->p_priority;	/* Queue to use. */
	register Proc **xpp;					/* Iterate over queue. */
	register Proc *prevXp;

	/* Side-effect for kernel: check if the task's stack still is ok? */
	if (isKernelProc(rp)) {
		if (*priv(rp)->s_stack_guard != STACK_GUARD)
		  panic("stack overrun by task", procNum(rp));
	}

	/* Now make sure that the process is not in its ready queue. Remove the
	 * process if it is found. A process can be made unready even if it is not
	 * running by being sent a signal that kills it.
	 */
	prevXp = NIL_PROC;
	for (xpp = &readyProcHead[queue]; *xpp != NIL_PROC; xpp = &(*xpp)->p_next_ready) {
		if (*xpp == rp) {		/* Found process to remove. */
			*xpp = (*xpp)->p_next_ready;		/* Replace with next chain. */
			if (rp == readyProcTail[queue])		/* Queue tail removed. */
			  readyProcTail[queue] = prevXp;	/* Set new tail. */
			if (rp == currProc || rp == nextProc)	/* Active process removed. */
			  pickProc();		/* Pick new process to run. */
			break;
		}
		prevXp = *xpp;			/* Save previous in chain. */					
	}
}

void lockEnqueue(Proc *rp) {	/* This process is now runnable */
	/* Safe gateway to enqueue() for tasks. */
	lock(3, "enqueue");
	enqueue(rp);
	unlock(3);
}

void lockDequeue(Proc *rp) {	/* This process is no longer runnable. */
	/* Safe gateway to dequeue() for tasks. */
	lock(4, "dequeue");
	dequeue(rp);
	unlock(4);
}

static int miniNotify(Proc *caller, int dst) {
	return OK;
}

/* src: sender of the notification
 * dst: who is to be notified
 */
int lockNotify(int src, int dst) {
	int result;
/* Safe gateway to miniNotify() for tasks and interrupt handlers. The sender
 * is explicitly given to prevent confusion where the call comes from. MINIX
 * kernel is not reentrant, which means to interrupts are disabled after
 * the first kernel entry (hardware interrupt, trap, or exception). Locking
 * is done by temporarily disabling interrupts.
 */
	if (kernelReentryCount >= 0) {
		result = miniNotify(procAddr(src), dst);
	} else {
		/* Call from task level, locking is required. */
		lock(0, "notify");
		result = miniNotify(procAddr(src), dst);
		unlock(0);
	}
	return result;
}
