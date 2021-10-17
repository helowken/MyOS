#define _TABLE

#include "kernel.h"
#include "proc.h"
#include "ipc.h"
#include "minix/com.h"

/* define Define stack sizes for the kernel tasks included in the system image. */
#define NO_STACK	0
#define SMALL_STACK	(128 * sizeof(char *))
#define IDL_S		SMALL_STACK		
#define HRD_S		NO_STACK		/* dummy task, uses kernel stack */
#define TSK_S		SMALL_STACK		/* system and clock task */

/* Stack space for all the task stacks. Declared as (char *) to align it. */
#define TOTAL_STACK_SPACE	(IDL_S + HRD_S + (2 * TSK_S))	
char *taskStack[TOTAL_STACK_SPACE / sizeof(char *)];

/* Define flags for the various process types. */
#define IDL_F	(SYS_PROC | PREEMPTIBLE | BILLABLE)	/* idle task */
#define TSK_F	(SYS_PROC)							/* Kernel tasks */
#define SRV_F	(SYS_PROC | PREEMPTIBLE)			/* System services */
#define USR_F	(BILLABLE | PREEMPTIBLE)			/* User processes */

/* Define system call traps for the various process types. These call masks
 * determine what system call traps a process is allowed to make.
 */
#define TSK_T	(1 << RECEIVE)						/* Clock and system */
#define SRV_T	(~0)								/* System services */
#define USR_T	((1 << SENDREC) | (1 << ECHO))		/* User processes */

/* Send masks determine to whom processes can send messages or notifications. */
#define s(n)	(1 << s_nrToId(n))
#define SRV_M	(~0)
#define SYS_M	(~0)
#define USR_M	(s(PM_PROC_NR) | s(FS_PROC_NR) | s(RS_PROC_NR))
#define DRV_M	(USR_M | s(SYSTEM) | s(CLOCK) | s(LOG_PROC_NR) | s(TTY_PROC_NR))

/* Define kernel calls that processes are allowed to make. */
#define c(n)	(1 << ((n) - KERNEL_CALL))
#define RS_C	~0
#define PM_C	~(c(SYS_DEVIO) | c(SYS_SDEVIO) | c(SYS_VDEVIO) | c(SYS_IRQCTL) \
					| c(SYS_INT86))
#define FS_C	(c(SYS_KILL) | c(SYS_VIRCOPY) | c(SYS_VIRVCOPY) | c(SYS_UMAP) \
					| c(SYS_GETINFO) | c(SYS_EXIT) | c(SYS_TIMES) | c(SYS_SETALARM))
#define DRV_C	(FS_C | c(SYS_SEGCTL) | c(SYS_IRQCTL) | c(SYS_INT86) | c(SYS_DEVIO) \
					| c(SYS_VDEVIO) | c(SYS_SDEVIO))
#define MEM_C	(DRV_C | c(SYS_PHYSCOPY) | c(SYS_PHYSVCOPY))

/* The system image table lists all programs that are part of the boot image.
 * The order of the entries here MUST agree with the order of the programs 
 * in the boot image and all kernel tasks must come first.
 * Each entry provides the process number, flags, quantum size (qs), scheduling
 * queue, allowed traps, ipc mask, and a name for the process table. The
 * initial program counter and stack size is also provided for kernel tasks.
 */
BootImage images[] = {
/*	  process nr,   pc, flags, qs,  queue, stack, traps, ipcTo,  call, name	    */
	{ IDLE,   idleTask, IDL_F,  8, IDLE_Q, IDL_S,	  0,	 0,	    0, "IDLE"   },
	{ CLOCK, clockTask,	TSK_F, 64, TASK_Q, TSK_S, TSK_T,	 0,	    0, "CLOCK"  },
	{ SYSTEM,  sysTask, TSK_F, 64, TASK_Q, TSK_S, TSK_T,     0,     0, "SYSTEM" },
	{ HARDWARE,		 0, TSK_F, 64, TASK_Q, HRD_S,     0,     0,	    0, "KERNEL" },
	{ PM_PROC_NR,    0, SRV_F, 32,      3, 0,     SRV_T, SRV_M,  PM_C, "pm"		}, 
	{ FS_PROC_NR,    0, SRV_F, 32,      4, 0,     SRV_T, SRV_M,  FS_C, "fs"		}, 
	{ RS_PROC_NR,    0, SRV_F,  4,      3, 0,     SRV_T, SYS_M,  RS_C, "rs"		}, 
	{ TTY_PROC_NR,   0, SRV_F,  4,      1, 0,     SRV_T, SYS_M, DRV_C, "tty"	}, 
	{ MEM_PROC_NR,   0, SRV_F,  4,      2, 0,     SRV_T, DRV_M, MEM_C, "memory"	}, 
	{ LOG_PROC_NR,   0, SRV_F,  4,      2, 0,     SRV_T, SYS_M, DRV_C, "log"	}, 
	{ DRVR_PROC_NR,  0, SRV_F,  4,      2, 0,     SRV_T, SYS_M, DRV_C, "driver"	}, 
	{ INIT_PROC_NR,  0, USR_F,  8, USER_Q, 0,     USR_T, USR_M,		0, "init"	},
};




