#ifndef PRIV_H
#define PRIV_H

#include "minix/com.h"
#include "protect.h"
#include "const.h"
#include "type.h"

typedef struct {
	proc_nr_t s_proc_nr;	/* Number of associated process */
	sys_id_t s_id;			/* Index of this system structure */
	short s_flags;			/* PREEMPTIBLE, BILLABLE, etc. */

	short s_trap_mask;		/* Allowed system call traps */
	SysMap s_ipc_from;		/* Allowed callers to receive from */
	SysMap s_ipc_to;		/* Allowed destination processes */
	long s_call_mask;		/* Allowed kernel calls */

	SysMap s_notify_pending;	/* Bit map with pending notifications. */
	irq_id_t s_int_pending;		/* Pending hardware interrupts */
	sigset_t s_sig_pending;		/* Pending signals */

	reg_t *s_stack_guard;	/* Stack guard word for kernel tasks */
} Priv;

/* Guard word for task stacks. */
#define STACK_GUARD		((reg_t) (sizeof(reg_t) == 2 ? 0xBEEF : 0xDEADBEEF))

/* Bits for the system property flags. */
#define PREEMPTIBLE		0x01	/* Kernel tasks are not preemptilble */
#define BILLABLE		0x04	/* Some processes are not billable */
#define SYS_PROC		0x10	/* System processes are privilieged */
#define SENDREC_BUSY	0x20	/* sendrec() in process */

/* Magic system structure table addresses. */
#define BEG_PRIV_ADDR	(&privTable[0])
#define END_PRIV_ADDR	(&privTable[NR_SYS_PROCS])

#define privAddr(i)		(privAddrTable)[(i)]
#define priv(rp)		((rp)->p_priv)
#define privId(rp)		(privAddr(rp)->s_id)
#define privIdToProcNum(id)		privAddr(id)->s_proc_nr

EXTERN Priv privTable[NR_SYS_PROCS];		/* System properties table */
EXTERN Priv *privAddrTable[NR_SYS_PROCS];

/* Unprivileged user processes all share the smae privilege structure.
 * This id must be fixed because it is used to check send mask entries.
 */
#define USER_PRIV_ID	0

#endif
