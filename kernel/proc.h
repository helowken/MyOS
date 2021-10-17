#ifndef PROC_H
#define PROC_H

#include "minix/com.h"
#include "protect.h"
#include "const.h"
#include "priv.h"

typedef struct {
	StackFrame p_reg;					/* Process' registers saved in stack frame */
	reg_t p_ldt_sel;					/* Selector in GDT with LDT base and limit */
	SegDesc p_ldt[2 + NR_REMOTE_SEGS];	/* CS, DS and remote segments */

	proc_nr_t p_nr;					/* Number of this process (for fast access) */
	char p_rt_flags;					/* Runtime flags, SENDING, RECEIVING, etc. */
} Proc;

/* Bits for the runtime flags. A process is rnnnable iff rtFlags == 0. */
#define SLOT_FREE		0x01			/* Process slot is free */

/* Scheduling priorities for priority. */
#define NR_SCHED_QUEUES	16		/* MUST equal minimum priority + 1 */
#define TASK_Q			0		/* Highest, used for kernel tasks */
#define MAX_USER_Q		0		/* Highest priority for user processes */
#define USER_Q			7		/* Default (should correspond to nice 0) */
#define MIN_USER_Q		14		/* Minimum priority for user processes */
#define IDLE_Q			15		/* Lowest, only IDLE process goes here */


/* Magic process table addresses. */
#define BEG_PROC_ADDR	(&procTable[0])
#define END_PROC_ADDR	(&procTable[NR_TASKS + NR_PROCS])


EXTERN Proc procTable[NR_TASKS + NR_PROCS];		/* Process table */
EXTERN Proc *procAddrTable[NR_TASKS + NR_PROCS];

#endif
