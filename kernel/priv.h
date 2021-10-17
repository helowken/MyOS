#ifndef PRIV_H
#define PRIV_H

#include "minix/com.h"
#include "protect.h"
#include "const.h"
#include "type.h"

typedef struct {
	proc_nr_t s_proc_nr;		/* Number of associated process. */
	sys_id_t s_id;			/* Index of this system structure. */
} Priv;

/* Bits for the system property flags. */
#define PREEMPTIBLE		0x01	/* Kernel tasks are not preemptilble */
#define BILLABLE		0x04	/* Some processes are not billable */
#define SYS_PROC		0x10	/* System processes are privilieged */
#define SENDREC_BUSY	0x20	/* sendrec() in process */


/* Magic system structure table addresses. */
#define BEG_PRIV_ADDR	(&privTable[0])
#define END_PRIV_ADDR	(&privTable[NR_SYS_PROCS])


EXTERN Priv privTable[NR_SYS_PROCS];		/* System properties table */
EXTERN Priv *privAddrTable[NR_SYS_PROCS];

#endif
