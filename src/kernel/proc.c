#include <minix/com.h>
#include "kernel.h"
#include "proc.h"

#define buildMsg(msg, src, dstProc)	\
	(msg)->m_source = (src);		\
	(msg)->m_type = NOTIFY_FROM(src);	\
	(msg)->NOTIFY_TIMESTAMP = getUptime();	\
	switch (src) {					\
		case HARDWARE:				\
			(msg)->NOTIFY_ARG = priv(dstProc)->s_int_pending;	\
			priv(dstProc)->s_int_pending = 0;	\
			break;					\
		case SYSTEM:				\
			(msg)->NOTIFY_ARG = priv(dstProc)->s_sig_pending;	\
			priv(dstProc)->s_sig_pending = 0;	\
			break;					\
	}

#define copyMsg(srcNum, srcProc, msg, dstProc, dstMsg)	\
	copyMessage(srcNum, (srcProc)->p_memmap[D].physAddr, (vir_bytes) msg,	\
				(dstProc)->p_memmap[D].physAddr, (vir_bytes) dstMsg)


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

/* caller: who is trying to send a message?
 * dst:	   to whom is message being sent?
 * msg:	   pointer to message buffer
 * flags:  system call flags
 */
static int miniSend(register Proc *caller, int dst, Message *msg, unsigned flags) {
/* Send a message from 'caller' to 'dst'. If 'dst' is blocked waiting
 * for this message, copy the message to it and unblock 'dst'. If 'dst' is
 * not waiting at all, or is waiting for another source, queue 'caller'.
 */
	register Proc *dstProc = procAddr(dst);
	register Proc **xpp;
	register Proc *xp;
	
	/* Check for deadlock by 'caller' and 'dst' sending to each other. */
	xp = dstProc;
	while (xp->p_rts_flags & SENDING) {		/* Check while sending */
		xp = procAddr(xp->p_send_to);		/* Get xp's destination */
		if (xp == caller)
		  return ELOCKED;		/* Deadlock if cyclic */
	}

	/* Check if 'dst' is blocked waiting for this message. The destination's
	 * SENDING flag may be set when its SENDREC call blocked while sending.
	 */
	if ( (dstProc->p_rts_flags & (RECEIVING | SENDING)) == RECEIVING &&
			(dstProc->p_get_from == ANY || dstProc->p_get_from == caller->p_nr)) {
		/* Destination is indeed waiting for this message. */
		copyMsg(caller->p_nr, caller, msg, dstProc, dstProc->p_msg);
		if ((dstProc->p_rts_flags &= ~RECEIVING) == 0)
		  enqueue(dstProc);
	} else if (! (flags & NON_BLOCKING)) {
		/* Destination is not waiting. Block and dequeue caller. */
		caller->p_msg = msg;
		if (caller->p_rts_flags == 0)
		  dequeue(caller);
		caller->p_rts_flags |= SENDING;
		caller->p_send_to = dst;

		/* Process is now blocked. Put it on the destination's queue. */
		xpp = &dstProc->p_sender_head;
		while (*xpp != NIL_PROC) {
			xpp = &(*xpp)->p_sender_next;
		}
		*xpp = caller;			/* Add caller to end. */
		caller->p_sender_next = NIL_PROC;	/* Mark new end of list. */
	} else  {
	  return ENOTREADY;
	}
	return OK;
}

/* caller: process trying to get message 
 * src:	   which message source is wanted
 * msg:	   pointer to message buffer
 * flags:  system call flags
 */
static int miniReceive(register Proc *caller, int src, Message *msg, unsigned flags) {
/* A process or task wants to get a message. If a message is already queued,
 * acquire it and deblock the sender. If no message from the desired source
 * is available block the caller, unless the flags don't allow blocking.
 */
	register Proc **xpp;
	register Proc *xp;
	Message recMsg;
	SysMap *map;
	bitchunk_t *chunk;
	int i, srcId, srcProcNum;
	
	/* Check to see if a message from desired source is already available.
	 * The caller's SENDING flag may be set if SENDREC couldn't send. If it
	 * is set, the process should be blocked.
	 */
	if (! (caller->p_rts_flags & SENDING)) {
		/* Check if there are pending notifications, except for SENDREC. */
		if (! (priv(caller)->s_flags & SENDREC_BUSY)) {
			map = &priv(caller)->s_notify_pending;
			for (chunk = &map->chunk[0]; chunk < &map->chunk[NR_SYS_CHUNKS]; ++chunk) {
				/* Find a pending notification from the requested source. */
				if (! *chunk)
				  continue;		/* No bits in chunk. */
				for (i = 0; ! (*chunk & (1 << i)); ++i) {
					/* Look up the bit. */
				}
				srcId = (chunk - &map->chunk[0]) * BITCHUNK_BITS + i;
				if (srcId >= NR_SYS_PROCS)
				  break;		/* Out of range. */
				srcProcNum = privIdToProcNum(srcId);	/* Get source proc num */
				if (src != ANY && src != srcProcNum)
				  continue;		/* Source not ok. */
				*chunk &= ~(1 << i);		/* No longer pending. */

				/* Found a suitable source, deliver the notification message. */
				buildMsg(&recMsg, srcProcNum, caller);	/* Assemble message. */
				copyMsg(srcProcNum, procAddr(HARDWARE), &recMsg, caller, msg);
				return OK;
			}
		}

		/* Check sender queue. */
		xpp = &caller->p_sender_head;
		while ((xp = *xpp) != NIL_PROC) {
			if (src == ANY || src == procNum(xp)) {
				/* Found acceptable message. Copy it and update status. */
				copyMsg(xp->p_nr, xp, xp->p_msg, caller, msg);
				if ((xp->p_rts_flags &= ~SENDING) == 0)
				  enqueue(xp);
				*xpp = xp->p_sender_next;	/* Remove from queue. */
				return OK;
			}
			xpp = &xp->p_sender_next;
		}
	}

	/* No suitable message is available or the caller couldn't send in SENDREC.
	 * Block the process trying to receive, unless the flags tell otherwise.
	 */
	if (! (flags & NON_BLOCKING)) {
		caller->p_get_from = src;
		caller->p_msg = msg;
		if (caller->p_rts_flags == 0)
		  dequeue(caller);
		caller->p_rts_flags |= RECEIVING;
		return OK;
	}
	return ENOTREADY;
}

/* caller:	sender of the notification
 * dst:	    which process to notify
 */
static int miniNotify(register Proc *caller, int dst) {
/* Check to see if target is blocked waiting for this message. A process
 * can be both sending and receiving during a SENDREC system call.
 */
	register Proc *dstProc = procAddr(dst);
	int srcId;
	Message notifyMsg;

	if ((dstProc->p_rts_flags & (SENDING | RECEIVING)) == RECEIVING &&
			! (priv(dstProc)->s_flags & SENDREC_BUSY) &&
			(dstProc->p_get_from == ANY || dstProc->p_get_from == caller->p_nr)) {
		/* Destination is indeed waiting for a message. Assemble a notification
		 * message and deliver it. Copy from pseudo-source HARDWARE, since the
		 * message is in the kernel's address space.
		 */
		buildMsg(&notifyMsg, procNum(caller), dstProc);
		copyMsg(procNum(caller), procAddr(HARDWARE), &notifyMsg, dstProc, dstProc->p_msg);
		dstProc->p_rts_flags &= ~RECEIVING;		/* Deblock destination. */
		if (dstProc->p_rts_flags == 0)
		  enqueue(dstProc);
		return OK;
	}

	/* Destination is not ready to receive the notification. Add it to the
	 * bit map with pending notifications. Note the indirectness: the system id
	 * instead of the process number is used in the pending bit map.
	 */
	srcId = priv(caller)->s_id;
	setSysBit(priv(dstProc)->s_notify_pending, srcId);
	return OK;
}

/* src: sender of the notification
 * dst: who is to be notified
 */
int lockNotify(int src, int dst) {
/* Safe gateway to miniNotify() for tasks and interrupt handlers. The sender
 * is explicitly given to prevent confusion where the call comes from. MINIX
 * kernel is not reentrant, which means to interrupts are disabled after
 * the first kernel entry (hardware interrupt, trap, or exception). Locking
 * is done by temporarily disabling interrupts.
 */
	int result;

	if (kernelReenter >= 0) {
		result = miniNotify(procAddr(src), dst);
	} else {
		/* Call from task level, locking is required. */
		lock(0, "notify");
		result = miniNotify(procAddr(src), dst);
		unlock(0);
	}
	return result;
}

int lockSend(int dst, Message *msg) {
/* Safe gateway to miniSend() for tasks. */
	int result;
	lock(2, "send");
	result = miniSend(currProc, dst, msg, NON_BLOCKING);
	unlock(2);
	return result;
}

int sys_call(int callNum, int srcDst, Message *msg) {
/* System calls are done by trapping to the kernel with an INT instruction.
 * The trap is caught and sys_call() is called to send or receive a message
 * (or both). The caller is always given by 'currProc'.
 */
	register Proc *caller = currProc;
	int function = callNum & SYSCALL_FUNC;	/* Get system call function */
	unsigned flags = callNum & SYSCALL_FLAGS;	/* Get flags */	
	int result;				/* The system call's result */

	/* Check if the process has privileges for the requested call. Calls to the
	 * kernel may only be SENDREC, because tasks always reply and may not block
	 * if the caller doesn't do receive().
	 */
	if (! (priv(caller)->s_trap_mask & (1 << function)) ||
			(isKernelNum(srcDst) && function != SENDREC && function != RECEIVE)) {
		kprintf("sys_call: trap %d not allowed, caller %d, src_dst %d\n", 
					function, procNum(caller), srcDst);
		return ECALLDENIED;		/* Trap denied by mask or kernel. */
	}

	/* Require a valid source and / or destination process, unless echoing. */
	if (! (isOkProcNum(srcDst) || srcDst == ANY || function == ECHO)) {
		kprintf("sys_call: invalid src_dst, src_dst %d, caller %d\n",
					srcDst, procNum(caller));
		return EBADSRCDST;		/* Invalid process number */
	}

	/* If the call is to send to a process, i.e., for SEND, SENDREC OR NOTIFY,
	 * verify that the caller is allowed to send to the given destination and
	 * that the destination is still alive.
	 */
	if (function & CHECK_DST) {
		if (! getSysBit(priv(caller)->s_ipc_to, procNumToPrivId(srcDst))) {
			kprintf("sys_call: ipc mask denied %d(%s) sending to %d\n",
						procNum(caller), caller->p_name, srcDst);
			return ECALLDENIED;
		}

		if (isEmptyProcNum(srcDst)) {
			if (!shutdownStarted) 
			  kprintf("sys_call: dead dst: %d->%d\n", procNum(caller), srcDst);
			return EDEADDST;
		}
	}

	/* Now check if the call is known and try to perform the request. The only
	 * system calls that exist in MINIX are sending and receiving messages.
	 *   - SENDREC: conbines SEND AND RECEIVE in a single system call
	 *   - SEND:	sender blocks until its message has been delivered
	 *   - RECEIVE:	receiver blocks until an acceptable message has arrived
	 *   - NOTIFY:	nonblocking call; deliver notification or mark pending
	 *   - ECHO:	nonblocking call; directly echo back the message
	 */
	switch (function) {
		case SENDREC:
			/* A flag is set so that notifications cannot interrupt SENDREC. */
			priv(caller)->s_flags |= SENDREC_BUSY;
			/* fall through */
		case SEND:
			result = miniSend(caller, srcDst, msg, flags);
			if (function == SEND || result != OK)
			  break;
		case RECEIVE:
			if (function == RECEIVE) 
			  priv(caller)->s_flags &= ~SENDREC_BUSY;
			result = miniReceive(caller, srcDst, msg, flags);
			break;
		case NOTIFY:
			result = miniNotify(caller, srcDst);
			break;
		case ECHO:
			copyMsg(caller->p_nr, caller, msg, caller, msg);
			result = OK;
			break;
		default:
			result = EBADCALL;		/* Illegal system call */
	}

	return result;
}
