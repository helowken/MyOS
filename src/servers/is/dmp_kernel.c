#include "is.h"
#include <timers.h>
#include <ibm/interrupt.h>
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"
#include "../../kernel/proc.h"
#include "../../kernel/ipc.h"

#define clickToRoundK(n) \
	((unsigned) ((((unsigned long) (n) << CLICK_SHIFT) + 512) / 1024))

/* Some global data that is shared among several dumping procedures.
 * Note that the process table copy has the same name as in the kernel
 * so that most macros and definitions from proc.h also apply here.
 */
Proc procTable[NR_TASKS + NR_PROCS];
Priv privTable[NR_SYS_PROCS];
BootImage images[NR_BOOT_PROCS];

static char *getProcName(int pNum) {
	if (pNum == ANY)
	  return "ANY";
	return cprocAddr(pNum)->p_name;
}
static char *procRtsFlagsStr(int flags) {
	static char str[10];

	str[0] = (flags & NO_MAP ? 'M' : '-');
	str[1] = (flags & SENDING ? 'S' : '-');
	str[2] = (flags & RECEIVING ? 'R' : '-');
	str[3] = (flags & SIGNALED ? 'S' : '-');
	str[4] = (flags & SIG_PENDING ? 'P' : '-');
	str[5] = (flags & P_STOP ? 'T' : '-');
	str[6] = '\0';
	return str;
}

void procTabDmp() {
/* Proc table dump */
	register Proc *rp;
	static Proc *oldRp = BEG_PROC_ADDR;
	int r, n = 0;
	phys_clicks textAddr, dataAddr, size;

	/* First obtain a fresh copy of the current process table. */
	if ((r = sysGetProcTab(procTable)) != OK) {
		report("IS", "warning: couldn't get copy of process table", r);
		return;
	}

	printf("\n--nr-name---- -prior-quant- -user---sys- -text---data---size- -rts flags-\n");

	for (rp = oldRp; rp < END_PROC_ADDR; ++rp) {
		if (isEmptyProc(rp))
		  continue;
		if (++n > 23)
		  break;

		textAddr = rp->p_memmap[T].physAddr;
		dataAddr = ACT_DATA_PADDR(rp->p_memmap);
		size = TOTAL_CLICKS(rp->p_memmap);
		if (procNum(rp) == IDLE || procNum(rp) < 0)
		  printf("(%2d) ", procNum(rp));
		else 
		  printf(" %2d  ", procNum(rp));

		printf(" %-8.8s %02u/%02u %02u/%02u %6lu%6lu %6uK%6uK%6uK %s",
					rp->p_name,
					rp->p_priority, rp->p_max_priority,
					rp->p_ticks_left, rp->p_quantum_size,
					rp->p_user_time, rp->p_sys_time,
					clickToRoundK(textAddr), clickToRoundK(dataAddr),
					clickToRoundK(size),
					procRtsFlagsStr(rp->p_rts_flags));

		if (rp->p_rts_flags & (SENDING | RECEIVING)) 
		  printf(" %-7.7s", getProcName(rp->p_get_from));

		printf("\n");
	}
	if (rp == END_PROC_ADDR)
	  rp = BEG_PROC_ADDR;
	else
	  printf("--more--\r");

	oldRp = rp;
}

void memmapDmp() {
	register Proc *rp;
	static Proc *oldRp = BEG_PROC_ADDR;
	int r, n = 0;
	phys_clicks size;

	/* First obtain a fresh copy of the current process table. */
	if ((r = sysGetProcTab(procTable)) != OK) {
		report("IS", "warning: couldn't get copy of process table", r);
		return;
	}

	printf("\n-nr/name--- --pc-- --sp-- -----text----- -----data----- ----stack----- --size-\n");
	for (rp = oldRp; rp < END_PROC_ADDR; ++rp) {
		if (isEmptyProc(rp))
		  continue;
		if (++n > 23)
		  break;
		size = TOTAL_CLICKS(rp->p_memmap);
		printf("%3d %-7.7s%7lx%7lx %4x %4x %4x %4x %4x %4x %4x %4x %4x %5uK\n",
					procNum(rp),
					rp->p_name,
					(unsigned long) rp->p_reg.pc,
					(unsigned long) rp->p_reg.esp,
					rp->p_memmap[T].virAddr, rp->p_memmap[T].physAddr, rp->p_memmap[T].len,
					rp->p_memmap[D].virAddr, ACT_DATA_PADDR(rp->p_memmap), rp->p_memmap[D].len,
					rp->p_memmap[S].virAddr, rp->p_memmap[S].physAddr, rp->p_memmap[S].len,
					clickToRoundK(size));
	}
	if (rp == END_PROC_ADDR)
	  rp = BEG_PROC_ADDR;
	else
	  printf("--more--\r");

	oldRp = rp;
}

static char *privFlagsStr(int flags) {
	static char str[10];

	str[0] = (flags & PREEMPTIBLE ? 'P' : '-');
	str[1] = '-';
	str[2] = (flags & BILLABLE ? 'B' : '-');
	str[3] = (flags & SYS_PROC ? 'S' : '-');
	str[4] = '-';
	str[5] = '\0';

	return str;
}

static char *privTrapsStr(int flags) {
	static char str[10];

	str[0] = (flags & (1 << ECHO) ? 'E' : '-');
	str[1] = (flags & (1 << SEND) ? 'S' : '-');
	str[2] = (flags & (1 << RECEIVE) ? 'R' : '-');
	str[3] = (flags & (1 << SENDREC) ? 'B' : '-');
	str[4] = (flags & (1 << NOTIFY) ? 'N' : '-');
	str[5] = '\0';

	return str;
}

void privilegesDmp() {
	register Proc *rp;
	static Proc *oldRp = BEG_PROC_ADDR;
	register Priv *sp;
	static char ipcTo[NR_SYS_PROCS + 1 + NR_SYS_PROCS / 8];
	int r, i, j, n = 0;

	/* First obtain a fresh copy of the current process and system table. */
	if ((r = sysGetPrivTab(privTable)) != OK) {
		report("IS", "warning: couldn't get copy of system privileges table", r);
		return;
	}
	if ((r = sysGetProcTab(procTable)) != OK) {
		report("IS", "warning: couldn't get copy of process table", r);
		return;
	}

	printf("\n--nr-id-name---- -flags- -traps- -ipc_to mask------------------------ \n");

	for (rp = oldRp; rp < END_PROC_ADDR; ++rp) {
		if (isEmptyProc(rp))
		  continue;
		if (++n > 23)
		  break;
		if (procNum(rp) == IDLE || procNum(rp) < 0)
		  printf("(%2d) ", procNum(rp));
		else
		  printf(" %2d  ", procNum(rp));

		r = -1;
		for (sp = &privTable[0]; sp < &privTable[NR_SYS_PROCS]; ++sp) {
			if (sp->s_proc_nr == rp->p_nr) {
				++r;
				break;
			}
		}
		if (r == -1 && ! (rp->p_rts_flags & SLOT_FREE)) 
		  sp = &privTable[USER_PRIV_ID];
		
		printf("(%02u) %-7.7s %s   %s  ",
					sp->s_id,
					rp->p_name,
					privFlagsStr(sp->s_flags),
					privTrapsStr(sp->s_trap_mask));

		for (i = j = 0; i < NR_SYS_PROCS; ++i, ++j) {
			ipcTo[j] = (getSysBit(sp->s_ipc_to, i) ? '1' : '0');
			if (i % 8 == 7)
			  ipcTo[++j] = ' ';
		}
		ipcTo[j] = '\0';

		printf(" %s \n", ipcTo);
	}
	if (rp == END_PROC_ADDR)
	  rp = BEG_PROC_ADDR;
	else
	  printf("--more--\r");

	oldRp = rp;
}

void imageDmp() {
	int m, i, j, r;
	BootImage *ip;
	static char ipcTo[BITCHUNK_BITS * 2];

	if ((r = sysGetImage(images)) != OK) {
		report("IS", "warning: couldn't get copy of image table", r);
		return;
	}
	printf("Image table dump showing all processes included in system image.\n");
	printf("---name-- -nr- -flags- -traps- -se- ----pc- -stack- -ipc_to[0]--------\n");
	for (m = 0; m < NR_BOOT_PROCS; ++m) {
		ip = &images[m];
		for (i = j = 0; i < BITCHUNK_BITS; ++i, ++j) {
			ipcTo[j] = (ip->ipcTo & (1 << i) ? '1' : '0');
			if (i % 8 == 7)
			  ipcTo[++j] = ' ';
		}
		ipcTo[j] = '\0';
		printf("%8s %4d   %s   %s  %3d %7lu %7lu   %s\n",
					ip->procName, ip->pNum,
					privFlagsStr(ip->flags), privTrapsStr(ip->trapMask),
					ip->priority, (long) ip->initialPC, ip->stackSize, ipcTo);
	}
	printf("\n");
}

void irqTabDmp() {
	int i, r;
	IrqHook irqHooks[NR_IRQ_HOOKS];
	IrqHook *e;		
	char *irq[] = {
		"clock",	/* 00 */
		"keyboard",	/* 01 */
		"cascade",  /* 02 */
		"rs232",    /* 03 */
		"rs232",	/* 04 */
		"NIC(eth)", /* 05 */
		"floppy",	/* 06 */
		"printer",	/* 07 */
		"",			/* 08 */
		"",			/* 09 */
		"",			/* 10 */
		"",			/* 11 */
		"",			/* 12 */
		"",			/* 13 */
		"at_wini_0",/* 14 */
		"at_wini_1"	/* 15 */
	};

	if ((r = sysGetIrqHooks(irqHooks)) != OK) {
		report("IS", "warning: couldn't get copy of irq hooks", r);
		return;
	}

	printf("IRQ policies dump shows use of kernel's IRQ hooks.\n");
	printf("-h.id- -proc.nr- -IRQ vector (nr.)- -policy- -notify id-\n");
	for (i = 0; i < NR_IRQ_HOOKS; ++i) {
		e = &irqHooks[i];
		printf("%3d", i);
		if (e->pNum == NONE) {
			printf("    <unused>\n");
			continue;
		}
		printf("%10d  ", e->pNum);
		printf("    %9.9s (%02d) ", irq[e->irq], e->irq);
		printf("  %s", (e->policy & IRQ_REENABLE ? "reenable" : "    -   "));
		printf("   %d\n", e->notifyId);
	}
	printf("\n");
}

void kernelMsgDmp() {
	KernelMsgs kMsgs;
	char printBuf[KMESS_BUF_SIZE + 1];
	int start;
	int r;

	/* Try to get a copy of the kernel messages. */
	if ((r = sysGetKernelMsgs(&kMsgs)) != OK) {
		printf("IS", "warning: couldn't get copy of kernelMsgs", r);
		return;
	}

	/* Try to print the kernel messages. First determine start and copy the
	 * buffer into a print-buffer. This is done because the messages in the
	 * copy may wrap (the kernel buffer is circular).
	 */
	start = ((kMsgs.km_next + KMESS_BUF_SIZE) - kMsgs.km_size) % KMESS_BUF_SIZE;
	r = 0;
	while (kMsgs.km_size > 0) {
		printBuf[r] = kMsgs.km_buf[(start + r) % KMESS_BUF_SIZE];
		++r;
		--kMsgs.km_size;
	}
	printBuf[r] = 0;		/* Make sure it terminates */
	printf("Dump of all messages generated by the kernel.\n\n");
	printf("%s", printBuf);	 
}

void scheduleDmp() {
	Proc *readyHead[NR_SCHED_QUEUES];
	KernelInfo kInfo;
	register Proc *rp;
	vir_bytes ptrDiff;
	int r;

	/* First obtain a scheduling information. */
	if ((r = sysGetSchedInfo(procTable, readyHead)) != OK) {
		report("IS", "warning: couldn't get copy of process table", r);
		return;
	}
	/* Then obtain kernel addresses to correct pointer information. */
	if ((r = sysGetKernelInfo(&kInfo)) != OK) {
		report("IS", "warning: couldn't get kernel addresses", r);
		return;
	}

	/* Update all pointers. Nasty pointer algorithmic ...
	 * ptrDiff: the diff between local and kernel.
	 */
	ptrDiff = (vir_bytes) procTable - (vir_bytes) kInfo.procTableAddr;

	for (r = 0; r < NR_SCHED_QUEUES; ++r) {
		if (readyHead[r] != NIL_PROC)
		  readyHead[r] = (Proc *) ((vir_bytes) readyHead[r] + ptrDiff);
	}
	for (rp = BEG_PROC_ADDR; rp < END_PROC_ADDR; ++rp) {
		if (rp->p_next_ready != NIL_PROC)
		  rp->p_next_ready = (Proc *) ((vir_bytes) rp->p_next_ready + ptrDiff);
	}

	/* Now show scheduling queues. */
	printf("Dumping scheduling queues.\n");

	for (r = 0; r < NR_SCHED_QUEUES; ++r) {
		rp = readyHead[r];
		if (! rp)
		  continue;
		printf("%2d: ", r);
		while (rp != NIL_PROC) {
			printf("%3d ", rp->p_nr);
			rp = rp->p_next_ready;
		}
		printf("\n");
	}
	printf("\n");
}

void monParamsDmp() {
	char val[1024];
	char *e;
	int r;

	/* Try to get a copy of the boot monitor parameters. */
	if ((r = sysGetMonParams(val, sizeof(val))) != OK) {
		report("IS", "warning: couldn't get copy of monitor params", r);
		return;
	}

	/* Append new lines to the result. */
	e = val;
	do {
		e += strlen(e);
		*e++ = '\n';
	} while (*e != 0);

	/* Finally, print the result. */
	printf("Dump of kernel environment strings set by boot monitor.\n");
	printf("\n%s\n", val);
}

void kernelEnvDmp() {
	KernelInfo kInfo;
	Machine machine;
	int r;

	if ((r = sysGetKernelInfo(&kInfo)) != OK) {
		report("IS", "warning: couldn't get copy of kernel info struct", r);
		return;
	}
	if ((r = sysGetMachine(&machine)) != OK) {
		report("IS", "warning: couldn't get copy of kernel machine struct", r);
		return;
	}

	printf("Dump of kernel info and machine structures. \n\n");
	printf("Machine structure:\n");
	printf("- pc_at:      %3d\n", machine.pc_at);
	printf("- ps_mca:     %3d\n", machine.ps_mca);
	printf("- processor:  %3d\n", machine.processor);
	printf("- vdu_ega:    %3d\n", machine.vdu_ega);
	printf("- vdu_vga:    %3d\n\n", machine.vdu_vga);

	printf("Kernel info structure:\n");
	printf("- code_base:     %5xH,  code_size:     %5xH\n", kInfo.codeBase, kInfo.codeSize);
	printf("- data_base:     %5xH,  data_size:     %5xH\n", kInfo.dataBase, kInfo.dataSize);
	printf("- proc_addr:     %5xH\n", kInfo.procTableAddr);
	printf("- kmem_base:     %5xH,  kmem_size:     %5xH\n", kInfo.kernelMemBase, kInfo.kernelMemSize);
	printf("- bootdev_base:  %5xH,  bootdev_size:  %5xH\n", kInfo.bootDevBase, kInfo.bootDevSize);
	//TODO printf("- bootdev_mem: \n");
	printf("- params_base:   %5xH,  params_size:   %5xH\n", kInfo.paramsBase, kInfo.paramsSize);
	printf("- nr_procs:   %3u\n", kInfo.numProcs);
	printf("- nr_tasks:   %3u\n", kInfo.numTasks);
	printf("- release:    %.6s\n", kInfo.release);
	printf("- version:    %.6s\n", kInfo.version);
	printf("\n");
}

void timingDmp() {
	printf("=== timingDmp\n");//TODO
}
