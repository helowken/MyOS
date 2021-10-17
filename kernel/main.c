#include "kernel.h"
#include "string.h"
#include "unistd.h"
#include "minix/com.h"
#include "proc.h"

void main() {
	BootImage *ip;		/* Boot image pointer */
	register Proc *rp;	/* Process pointer */
	register Priv *sp;	/* Privilege pointer */
	register int i;
	reg_t kernelTaskStackBase;

	/* Initialize the interrupt controller. */
	initInterrupts();

	/*
	 * Clear the process table. Anounce each slot as empty.
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
		privAddrTable[i] = sp;	/* Priv ptr from number. */
	}

	kernelTaskStackBase = (reg_t) taskStack;
}
