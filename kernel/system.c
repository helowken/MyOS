#include "kernel.h"
#include "system.h"
#include "stdlib.h"
#include "unistd.h"
#include "protect.h"
#include "signal.h"
#include "ibm/memory.h"

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
		irqHooks[i].pNum = NONE;
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
	map(SYS_GETKSIG, doGetKernelSig);
	map(SYS_ENDKSIG, doEndKernelSig);
	map(SYS_SIGSEND, doSigSend);
	map(SYS_SIGRETURN, doSigReturn);

	/* Device I/O. */
	map(SYS_IRQCTL, doIrqCtl);
	map(SYS_DEVIO, doDevIO);
	/*
	map(SYS_SDEVIO, doStrDevIO);
	map(SYS_VDEVIO, doVecDevIO);
	map(SYS_INT86, doInt86);
	*/

	/* Memory management. */
	/*
	map(SYS_NEWMAP, doNewMap);
	map(SYS_SEGCTL, doSegCtl);
	*/
	map(SYS_MEMSET, doMemset);
	
	/* Copying. */
	map(SYS_VIRCOPY, doVirCopy);
	map(SYS_PHYSCOPY, doPhysCopy);
	map(SYS_UMAP, doUMap);
	/*
	map(SYS_VIRVCOPY, doVirVecCopy);
	map(SYS_PHYSVCOPY, doPhysVecCopy);
	*/

	/* Clock functionality. */
	map(SYS_TIMES, doTimes);
	map(SYS_SETALARM, doSetAlarm);

	/* System control. */
	//map(SYS_ABORT, doAbort);
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

phys_bytes umapLocal(Proc *rp, int seg, vir_bytes virAddr, vir_bytes bytes) {
/* Calculate the physical memory address for a given virtual address. */
	vir_clicks virAddrClicks;		/* The virtual address in clicks */
	vir_clicks segEndClicks;

	if (bytes <= 0)
	  return (phys_bytes) 0;
	if (virAddr + bytes <= virAddr)
	  return 0;		/* Overflow */
	virAddrClicks = (virAddr + bytes - 1) >> CLICK_SHIFT;	/* Last click of data */

	if (seg != T) 
	  seg = (virAddrClicks < rp->p_memmap[D].virAddr + rp->p_memmap[D].len ? D : S);

	segEndClicks = rp->p_memmap[seg].virAddr + rp->p_memmap[seg].len;

	if ((virAddr >> CLICK_SHIFT) >= segEndClicks || virAddrClicks >= segEndClicks)
	  return (phys_bytes) 0;

	return (rp->p_memmap[seg].physAddr << CLICK_SHIFT) + virAddr -
			(rp->p_memmap[seg].virAddr << CLICK_SHIFT);
}

phys_bytes umapRemote(Proc *rp, int seg, vir_bytes virAddr, vir_bytes bytes) {
/* Calculate the physical memory address for a given virtual address. */
	FarMem *fm;

	if (bytes <= 0)
	  return 0;
	if (seg < 0 || seg >= NR_REMOTE_SEGS)
	  return 0;

	fm = &rp->p_priv->s_far_mem[seg];
	if (! fm->inUse)
	  return 0;
	if (virAddr + bytes > fm->len)
	  return 0;
	return fm->physAddr + (phys_bytes) virAddr;
}

void sendSig(int pNum, int sig) {
/* Notify a system process about a signal. This is straightforward. Simply
 * set the signal that is to be delivered in the pending signals map and
 * send a notification with source SYSTEM.
 */
	register Proc *rp;

	rp = procAddr(pNum);
	sigaddset(&priv(rp)->s_sig_pending, sig);
	lockNotify(SYSTEM, pNum);
}

void causeSig(int pNum, int sig) {
/* A system process wants to send a signal to a process. Examples are:
 *	- HARDWARE wanting to cause a SIGSEGV after a CPU exception
 *	- TTY wanting to cause SIGINT upon getting a DEL
 *	- FS wanting to cause SIGPIPE for a broken pipe
 * Signals are handled by sending a message to PM. This function handles the
 * signals and makes sure the PM gets them by sending a notification. The
 * process being signaled is blocked while PM has not finished all signals
 * for it.
 * Race conditions between calls to this function and the system calls that
 * process pending kernel signals cannot exist. Signal related functions are
 * only called when a user process causes a CPU exception and from the kernel
 * process level, which runs to completion.
 */
	register Proc *rp;

	/* Check if the signal is already pending. Process it otherwise. */
	rp = procAddr(pNum);
	if (! sigismember(&rp->p_pending, sig)) {
		sigaddset(&rp->p_pending, sig);
		if (! (rp->p_rt_flags & SIGNALED)) {		/* Other pending */
			if (rp->p_rt_flags == 0)
			  lockDequeue(rp);		/* Make not ready */
			rp->p_rt_flags |= SIGNALED | SIG_PENDING;	/* Update flags */
			sendSig(pNum, sig);
		}
	}
}

phys_bytes umapBios(register Proc *rp, vir_bytes virAddr, vir_bytes bytes) {
/* Calculate the physical memory address at the BIOS. Note: currently, BIOS
 * address zero (the first BIOS interrupt vector) is not considered as an
 * error here, but since the physical address will be zero as well, the
 * calling function will think an error occurred. This is not a problem,
 * since no one uses the first BIOS interrupt vector.
 */

	/* Check all acceptable ranges. */
	if (virAddr >= BIOS_MEM_BEGIN && virAddr + bytes <= BIOS_MEM_END)
	  return (phys_bytes) virAddr;
	else if (virAddr >= BASE_MEM_TOP && virAddr + bytes <= UPPER_MEM_END)
	  return (phys_bytes) virAddr;
	kprintf("Warning, error in umapBios, virtual address 0x%x\n", virAddr);
	return 0;
}

int virtualCopy(VirAddr *srcAddr, VirAddr *dstAddr, vir_bytes bytes) {
/* Copy bytes from virtual address srcAddr to virtual address dstAddr.
 * Virtual addresses can be in ABS, LOCAL_SEG, REMOTE_SEG, or BIOS_SEG.
 */
	VirAddr *virAddr[2];	/* Virtual source and destination address */
	phys_bytes physAddr[2];	/* Absolute source and destination */
	int segIdx;
	int i;

	/* Check copy count. */
	if (bytes <= 0)
	  return EDOM;

	/* Do some more checks and map virtual addresses to physical addresses. */
	virAddr[_SRC_] = srcAddr;
	virAddr[_DST_] = dstAddr;
	for (i = _SRC_; i <= _DST_; ++i) {
		/* Get physical address. */
		switch (virAddr[i]->segment & SEGMENT_TYPE) {
			case LOCAL_SEG:
				segIdx = virAddr[i]->segment & SEGMENT_INDEX;
				physAddr[i] = umapLocal(procAddr(virAddr[i]->pNum), segIdx,
							virAddr[i]->offset, bytes);
				break;
			// case REMOTE_SEG: TODO
			case BIOS_SEG:
				physAddr[i] = umapBios(procAddr(virAddr[i]->pNum), 
								virAddr[i]->offset, bytes);
				break;
			case PHYS_SEG:	
				physAddr[i] = virAddr[i]->offset;
				break;
			default:
				return EINVAL;
		}

		/* Check if mapping succeeded. */
		if (physAddr[i] <= 0 && virAddr[i]->segment != PHYS_SEG)
		  return EFAULT;
	}

	/* Now copy bytes between physical addresses */
	physCopy(physAddr[_SRC_], physAddr[_DST_], bytes);
	return OK;
}

void getRandomness(int source) {
/* On machines with the RDTSC (cycle counter read instruction - pentium
 * and up), use that for high-resolution raw entropy gathering. Otherwise,
 * use the realtime clock (tick resolution).
 *
 * Unfortunately this test is run-time - we don't want to bother with
 * compiling different kernels for different machines.
 *
 * On machines without RDTSC, we use readClock().
 */
	int rNext;
	unsigned long tscHigh, tscLow;

	source %= RANDOM_SOURCES;
	rNext = kernelRandom.bin[source].r_next;
	if (machine.processor > 486) {
		readTsc(&tscHigh, &tscLow);
		kernelRandom.bin[source].r_buf[rNext] = tscLow;
	} else {
		kernelRandom.bin[source].r_buf[rNext] = readClock();
	}
	if (kernelRandom.bin[source].r_size < RANDOM_ELEMENTS) {
		++kernelRandom.bin[source].r_size;
	}
	kernelRandom.bin[source].r_next = (rNext + 1) % RANDOM_ELEMENTS;
}

