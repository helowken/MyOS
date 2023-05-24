#include "kernel.h"
#include "proc.h"
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <image.h>
#include <minix/com.h>

static void announce() {
	kprintf("\nMINIX %s.%s. ", OS_RELEASE, OS_VERSION);
	kprintf("Executing in 32a-bit protected mode.\n\n");
}

void main() {
	BootImage *ip;		/* Boot image pointer */
	register Proc *rp;	/* Process pointer */
	register Priv *sp;	/* Privilege pointer */
	register int i;
	int hdrIdx;
	phys_clicks textBase, textBytes, dataBase;
	vir_clicks textClicks, dataClicks, offsetClicks;
	reg_t kernelTaskStackBase;
	Exec exec;
	Elf32_Phdr *hdr;

	/* Initialize the interrupt controller. */
	initInterrupts(1);

	/* Clear the process table. Anounce each slot as empty.
	 * Do the same for the table with privilege structures for the system processes.
	 */
	for (rp = BEG_PROC_ADDR, i = -NR_TASKS; rp < END_PROC_ADDR; ++rp, ++i) {
		rp->p_rts_flags = SLOT_FREE;			/* Initialize free slot. */
		rp->p_nr = i;							/* Proc number from ptr. */
		(procAddrTable + NR_TASKS)[i] = rp;		/* Proc ptr from number. */
	}
	for (sp = BEG_PRIV_ADDR, i = 0; sp < END_PRIV_ADDR; ++sp, ++i) {
		sp->s_proc_nr = NONE;		/* Initialize as free. */
		sp->s_id = i;				/* Priv structure index. */
		privAddrTable[i] = sp;		/* Priv ptr from number. */
	}

	/* Task stacks. */
	kernelTaskStackBase = (reg_t) taskStack;

	for (i = 0; i < NR_BOOT_PROCS; ++i) { 
		ip = &images[i];					/* Process' attributes */			
		rp = procAddr(ip->pNum);			/* Get process pointer */
		rp->p_max_priority = ip->priority;	/* Max scheduling priority */
		rp->p_priority = ip->priority;		/* Current priority */
		rp->p_quantum_size = ip->quantum;	/* Quantum size in ticks */
		rp->p_ticks_left = ip->quantum;		/* Current credit */
		strncpy(rp->p_name, ip->procName, P_NAME_LEN);	/* Set process name */
		getPriv(rp, (ip->flags & SYS_PROC));	/* Assign privileged structure */

		sp = priv(rp);
		sp->s_flags = ip->flags;			/* Process flags */
		sp->s_trap_mask = ip->trapMask;		/* Allowed traps */
		sp->s_call_mask = ip->callMask;		/* Kernel call mask */
		sp->s_ipc_to.chunk[0] = ip->ipcTo;	/* Restrict targets */

		if (isKernelProc(rp)) {				/* Is Part of the kernel? */
			if (ip->stackSize > 0) {		/* HARDWARE stack size is 0 */
				sp->s_stack_guard = (reg_t *) kernelTaskStackBase;
				*((reg_t *) kernelTaskStackBase) = STACK_GUARD;
			}
			kernelTaskStackBase += ip->stackSize;	/* Point to high end of the stack */
			rp->p_reg.esp = kernelTaskStackBase;	/* This stack's initial stack ptr */
			hdrIdx = 0;						/* All use the first image header */
		} else {
			hdrIdx = 1 + i - NR_TASKS;		/* Servers, drivers, INIT */
		}

		/* The bootstrap loader created an array of image headers at 
		 * absolute address execPos. Get one element to exec.
		 */
		physCopy(imgHdrPos + hdrIdx * EXEC_SIZE, vir2Phys(&exec), 
					(phys_bytes) EXEC_SIZE);

		/* Build process memory map */
		hdr = &exec.codeHdr;
		textBase = hdr->p_paddr >> CLICK_SHIFT;
		textBytes = hdr->p_memsz;
		textClicks = (textBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;

		hdr = &exec.dataHdr;
		dataBase = hdr->p_paddr >> CLICK_SHIFT;
		dataClicks = (hdr->p_vaddr + hdr->p_memsz + exec.stackHdr.p_memsz 
						+ CLICK_SIZE - 1) >> CLICK_SHIFT;
		offsetClicks = hdr->p_offset >> CLICK_SHIFT;

		rp->p_memmap[T].physAddr = textBase;
		rp->p_memmap[T].len = textClicks;
		/* [D].virAddr is always 0. */
		rp->p_memmap[D].physAddr = dataBase;	/* data virAddr = 0 */
		rp->p_memmap[D].len = dataClicks;
		rp->p_memmap[D].offset = offsetClicks;
		/* [S].len is empty(len = 0) since stack is in data */
		rp->p_memmap[S].physAddr = rp->p_memmap[D].physAddr + dataClicks;
		rp->p_memmap[S].virAddr = rp->p_memmap[D].virAddr + dataClicks;	

		/* Set initial register values. The Proessor status word for tasks
		 * is different from that of other processes because tasks can
		 * access I/O; this is not allowed to less-privileged processes.
		 */
		rp->p_reg.pc = (reg_t) ip->initialPC;
		rp->p_reg.psw = (isKernelProc(rp)) ? INIT_TASK_PSW : INIT_PSW;

		/* Initialize the server stack pointer. Take it down one word
		 * to give crtso.s something to use as "argc"
		 */
		if (isUserProc(rp)) {		/* Is user-space process? */
			rp->p_reg.esp = (rp->p_memmap[S].virAddr + rp->p_memmap[S].len) << CLICK_SHIFT;
			rp->p_reg.esp -= sizeof(reg_t);
		}
		
		/* Set ready. The HARDWARE task is never ready. */
		if (rp->p_nr != HARDWARE) {
			rp->p_rts_flags = 0;		/* Runnable if no flags */
			lockEnqueue(rp);	/* add to scheduling queues */
		} else {
			rp->p_rts_flags = NO_MAP;
		}

		/* Code and data segments must be allocated in protected mode. */
		allocSegments(rp);
	}

	// TODO boot device

	/* We're definitely not shutting down. */
	shutdownStarted = 0;

	/* MINIX is now ready. All boot image processes are on the ready queue.
	 * Return to the assembly code to start running the current process.
	 */
	billProc = procAddr(IDLE);	/* It has to point somewhere */
	announce();					/* Print MINIX startup banner */
	restart();
}

static void shutdown(Timer *tp) {
/* This function is called from prepareShutdown to bring down MINIX. 
 * How to shutdown is in the argument: RBT_HALT (return to the
 * monitor), RBT_MONITOR (execute given code), RBT_RESET (hard reset).
 */
	int how = timerArg(tp)->ta_int;
	u16_t magic;

	/* Now mask all interrupts, including the clock, and stop the clock. */
	outb(INT_CTL_MASK, ~0);
	clockStop();

	if (how != RBT_RESET) {
		/* Reinitialize the interrupt controllers to the BIOS defaults. */
		initInterrupts(0);
		outb(INT_CTL_MASK, 0);
		outb(INT2_CTL_MASK, 0);

		/* Return to the boot monitor. Set the program if not already done. */
		if (how != RBT_MONITOR) {
			/* Before calling minix(), we copy the params from boot envs, and
			 * call name2Dev() to transform the dev to a number. 
			 * After returning to monitor, we call parseCode(params), now the 
			 * params contains a number for the dev, it's incorrect, and can
			 * not boot again.
			 * So if we need to make the params to be a empty string.
			 */
			physCopy(vir2Phys(""), kernelInfo.paramsBase, 1);	
		}

		level0(monitor);
	}

	/* Reset the system by jumping to the reset address (real mode), or by
	 * forcing a processor shutdown (protected mode). First stop the BIOS
	 * memory test by setting a soft reset flag.
	 */
	magic = STOP_MEM_CHECK;
	physCopy(vir2Phys(&magic), SOFT_RESET_FLAG_ADDR, SOFT_RESET_FLAG_SIZE);
	level0(reset);
}

void prepareShutdown(int how) {
/* This function prepares to shutdown MINIX. */
	static Timer shutdownTimer;
	register Proc *rp;
	Message msg;

	/* Show debugging dumps on panics. Make sure that the TTY driver is still
	 * available to handle them. This is done with help of a non-blocking send.
	 * We rely on TTY to call sysAbort() when it is done with the dumps.
	 */
	if (how == RBT_PANIC) {
		msg.m_type = PANIC_DUMPS;
		if (nbSend(TTY_PROC_NR, &msg) == OK)	/* Don't block if TTY isn't ready */
		  return;				/* Await sysAbort() from TTY */
	}

	/* Send a signal to all system processes that are still alive to inform
	 * them that the MINIX kernel is shutting down. A proper shutdown sequence
	 * should be implemented by a user-space server. This mechanism is useful
	 * as a backup in caes of system panics, so that system processes can stll
	 * run their shutdown code, e.g, to synchronize the FS or to let the TTY
	 * switch to the first console.
	 */
	kprintf("Sending SIGKSTOP to system processes ...\n");
	for (rp = BEG_PROC_ADDR; rp < END_PROC_ADDR; ++rp) {
		if (! isEmptyProc(rp) && 
					(priv(rp)->s_flags & SYS_PROC) && 
					! isKernelProc(rp))
			sendSig(procNum(rp), SIGKSTOP);
	}

	/* We're shutting down. Diagnostics may behave differently now. */
	shutdownStarted = 1;

	/* Notify system processes of the upcoming shutdown and allow them to be
	 * scheduled by setting a watchdog timer that calls shutdown(). The timer
	 * argument passes the shutdown status.
	 */
	kprintf("MINIX will now be shut down ...\n");
	timerArg(&shutdownTimer)->ta_int = how;

	/* Continue after 1 second, to give processes a chance to get
	 * scheduled to do shutdown work.
	 */
	setTimer(&shutdownTimer, getUptime() + HZ, shutdown);
}



