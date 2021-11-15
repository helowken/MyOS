#include "kernel.h"
#include "string.h"
#include "unistd.h"
#include "image.h"
#include "minix/com.h"
#include "proc.h"

static void announce() {
	kprintf("\nMINIX %s.%s. ", OS_RELEASE, OS_VERSION);
	kprintf("Executing in 32-bit protected mode.\n\n");
}

void main() {
	BootImage *ip;		/* Boot image pointer */
	register Proc *rp;	/* Process pointer */
	register Priv *sp;	/* Privilege pointer */
	register int i;
	int hdrIdx;
	phys_bytes textBase = 0, dataBase = 0;
	vir_bytes textLen = 0, dataLen = 0;
	reg_t kernelTaskStackBase;
	Exec imgHdr;
	Elf32_Phdr *hdr;

	/* Initialize the interrupt controller. */
	initInterrupts();

	/* Clear the process table. Anounce each slot as empty.
	 * Do the same for the table with privilege structures for the system processes.
	 */
	for (rp = BEG_PROC_ADDR, i = -NR_TASKS; rp < END_PROC_ADDR; ++rp, ++i) {
		rp->p_rt_flags = SLOT_FREE;				/* Initialize free slot. */
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
		rp = procAddrTable[ip->procNum];	/* Get process pointer */
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
		 * absolute address imgHdrPos. Get one element to imgHdr.
		 */
		physCopy(imgHdrPos + hdrIdx * EXEC_SIZE, vir2Phys(&imgHdr), 
					(phys_bytes) EXEC_SIZE);

		/* Build process memory map */
		hdr = &imgHdr.codeHdr;
		if (isPLoad(hdr)) {
			textBase = hdr->p_paddr;
			textLen = hdr->p_memsz;
		}
		hdr = &imgHdr.dataHdr;
		if (isPLoad(hdr)) {
			dataBase = hdr->p_paddr;
			dataLen = hdr->p_vaddr + hdr->p_memsz;
		}
		rp->p_memmap[T].physAddr = textBase;
		rp->p_memmap[T].len = textLen;
		rp->p_memmap[D].physAddr = dataBase;
		rp->p_memmap[D].len = dataLen;
		rp->p_memmap[S].physAddr = dataBase + dataLen;
		rp->p_memmap[S].len = 0;

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
			rp->p_reg.esp = rp->p_memmap[S].physAddr - sizeof(reg_t);
		}
		
		/* Set ready. The HARDWARE task is never ready. */
		if (rp->p_nr != HARDWARE) {
			rp->p_rt_flags = 0;		/* Runnable if no flags */
			lockEnqueue(rp);	/* add to scheduling queues */
		} else {
			rp->p_rt_flags = NO_MAP;
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
