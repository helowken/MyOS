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

	MemMap p_memmap[NR_LOCAL_SEGS];	/* Memory map (T, D, S) */ 

	clock_t p_user_time;	/* User time in ticks */
	clock_t p_sys_time;		/* Sys time in ticks */

	struct Proc *p_next_ready;	/* Pointer to next ready process */
	struct Proc *p_sender_head;	/* Head of list of procs wishing to send */
	struct Proc *p_sender_next;	/* Link to next proc wishing to send */
	Message *p_msg;			/* Pointer to passed message buffer */
	proc_nr_t p_get_from;	/* From whom does process want to receive? */
	proc_nr_t p_send_to;	/* To whom does process want to send? */

	sigset_t p_pending;		/* Bit map for pending kernel signals */

	char p_name[P_NAME_LEN];	/* Name of the process, including '\0' */	
} Proc;

/* Bits for the runtime flags. A process is rnnnable iff p_rt_flags == 0. */
#define SLOT_FREE		0x01	/* Process slot is free */
#define NO_MAP			0x02	/* Keeps unmapped forked child from running */
#define SENDING			0x04	/* Process blocked trying to SEND */
#define RECEIVING		0x08	/* Process blocked trying to RECEIVE */
#define SIGNALED		0x10	/* Set when new kernel signal arrives */
#define SIG_PENDING		0x20	/* Unready while signal being processed */
#define P_STOP			0x40	/* Set when process is being traced */
#define NO_PRIV			0x80	/* Keep forked system process from running */

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

#define isOkProcNum(n)		((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS)
#define isEmptyProcNum(n)	isEmptyProc(procAddr(n))
#define isEmptyProc(p)		((p)->p_rt_flags == SLOT_FREE)
#define isKernelProc(p)		isKernelNum((p)->p_nr)
#define isKernelNum(n)		((n) < 0)
#define isUserProc(p)		isUserNum((p)->p_nr)
#define isUserNum(n)		((n) >= 0)

EXTERN Proc procTable[NR_TASKS + NR_PROCS];		/* Process table */
EXTERN Proc *procAddrTable[NR_TASKS + NR_PROCS];
EXTERN Proc *readyProcHead[NR_SCHED_QUEUES];	/* Ptrs to ready list headers */
EXTERN Proc *readyProcTail[NR_SCHED_QUEUES];	/* Ptrs to read list tails */

#endif
