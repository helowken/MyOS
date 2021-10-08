#ifndef PROC_H
#define PROC_H

#include "minix/com.h"
#include "protect.h"
#include "const.h"

typedef struct {
	StackFrame reg;					/* Process' registers saved in stack frame */
	reg_t ldtSel;						/* Selector in GDT with LDT base and limit */
	SegDesc ldt[2 + NR_REMOTE_SEGS];	/* CS, DS and remote segments */
} Proc;


/* Magic process table addresses. */
#define BEG_PROC_ADDR	(&proc[0])
#define END_PROC_ADDR	(&proc[NR_TASKS + NR_PROCS])


EXTERN Proc proc[NR_TASKS + NR_PROCS];	/* Process table */


#endif
