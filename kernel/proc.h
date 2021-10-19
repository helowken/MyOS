#ifndef PROC_H
#define PROC_H

#include "minix/com.h"
#include "protect.h"
#include "const.h"
#include "priv.h"

typedef struct Proc {
	StackFrame p_reg;		/* Process' registers saved in stack frame */
	reg_t p_ldt_sel;		/* Selector in GDT with LDT base and limit */
	SegDesc p_ldt[2 + NR_REMOTE_SEGS];	/* CS, DS and remote segments */

	proc_nr_t p_nr;			/* Number of this process (for fast access) */
	Priv *p_priv;			/* System privilege structure */
	char p_rt_flags;		/* Runtime flags, SENDING, RECEIVING, etc. */

	char p_priority;		/* Current scheduling priority */		
	char p_max_priority;	/* Maximum scheduling priority */
	char p_ticks_left;		/* Number of scheduling ticks left */
	char p_quantum_size;	/* Quantum size in ticks */

	char p_name[P_NAME_LEN];	/* Name of the process, including '\0' */	
} Proc;

/* Bits for the runtime flags. A process is rnnnable iff p_rt_flags == 0. */
#define SLOT_FREE		0x01	/* Process slot is free */
#define NO_MAP			0x02	/* Keeps unmapped forked child from running */

/* Scheduling priorities for priority. */
#define NR_SCHED_QUEUES	16		/* MUST equal minimum priority + 1 */
#define TASK_Q			0		/* Highest, used for kernel tasks */
#define MAX_USER_Q		0		/* Highest priority for user processes */
#define USER_Q			7		/* Default (should correspond to nice 0) */
#define MIN_USER_Q		14		/* Minimum priority for user processes */
#define IDLE_Q			15		/* Lowest, only IDLE process goes here */

/* Magic process table addresses. */
#define BEG_PROC_ADDR	(&procTable[0])
#define BEG_USER_ADDR	(&procTable[NR_TASKS])
#define END_PROC_ADDR	(&procTable[NR_TASKS + NR_PROCS])

#define NIL_PROC		((Proc *) 0)
#define NIL_SYS_PROC	((Proc *) 1)
#define procAddr(n)		(procAddrTable + NR_TASKS)[(n)]
#define procNum(p)		((p)->p_nr)

#define isKernelProc(p)	isKernelNum((p)->p_nr)
#define isKernelNum(n)	((n) < 0)
#define isUserProc(n)	isUserNum((p)->p_nr)
#define isUserNum(n)	((n) >= 0)

EXTERN Proc procTable[NR_TASKS + NR_PROCS];		/* Process table */
EXTERN Proc *procAddrTable[NR_TASKS + NR_PROCS];

#endif
