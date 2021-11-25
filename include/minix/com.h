#ifndef _MINIX_COM_H
#define _MINIX_COM_H

#define arraySize(a)		(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)		((a) + arraySize(a))

/*=====================================================================*
 *                Magic process numbers                *
 *=====================================================================*/
#define ANY			0x7ace	/* Used to indicate 'any process' */
#define NONE		0x6ace	/* Used to indicate 'no process at all' */
#define SELF		0x8ace	/* Used to indicate 'own process' */

/*=====================================================================*
 *      Process numbers of processes in the system image      *
 *=====================================================================*/

/*Kernel tasks. These all run in the same address space. */
#define IDLE			-4	/* Runs when no one else can run. */
#define CLOCK			-3	/* Alarms and other clock functions. */
#define SYSTEM			-2	/* Request system functionality. */
#define KERNEL			-1	/* Pseudo-process for IPC and scheduling. */
#define HARDWARE	KERNEL	/* For hardware interrupt handlers. */

/* Numer of tasks. Note that NR_PROCS is defined in <minix/config.h>. */
#define	NR_TASKS		4

/* User-space processes, that is, device drivers, servers, and INIT. */
#define PM_PROC_NR		0	/* Process manager */
#define FS_PROC_NR		1	/* File system */
#define RS_PROC_NR		2	/* Reincarnation server */
#define MEM_PROC_NR		3	/* Memory driver (RAM disk, null, etc.) */
#define LOG_PROC_NR		4	/* Log device driver */
#define TTY_PROC_NR		5	/* Terminal (TTY) driver */
#define DRVR_PROC_NR	6	/* Device driver for boot medium */
#define INIT_PROC_NR	7	/* Init -- goes multiuser */

/* Number of processes contained in the system image. */
#define NR_BOOT_PROCS	(NR_TASKS + INIT_PROC_NR + 1)

/*=====================================================================*
 *			Kernel notification types			*
 *=====================================================================*/

/* Kernel notification types. In principle, these can be sent to any process,
 * so make sure that these types do not interfere with other message types.
 * Notifications are prioritized because of the way they are unhold() and
 * blocking notifications are delivered. The lowest numbers go first. The
 * offset are used for the per-process notification bit maps.
 */
#define NOTIFY_MESSAGE	0x1000	
#define NOTIFY_FROM(procNum)	(NOTIFY_MESSAGE | ((procNum) + NR_TASKS))
#define SYNC_ALARM	NOTIFY_FROM(CLOCK)		/* Synchronous alarm */
#define HARD_INT	NOTIFY_FROM(HARDWARE)	/* Hardware interrupt */

/*=====================================================================*
 *			SYSTASK	request types and field names			*
 *=====================================================================*/

/* System library calls are dispatched via a call vector. 
 * The numbers here determine which call is made from the call vector.*/
#define KERNEL_CALL	0X600	/* Base for kernel calls to SYSTEM */

#define SYS_FORK		(KERNEL_CALL + 0)	/* sys_fork() */
#define SYS_EXEC		(KERNEL_CALL + 1)	/* sys_exec() */
#define SYS_EXIT		(KERNEL_CALL + 2)	/* sys_exit() */
#define SYS_NICE		(KERNEL_CALL + 3)	/* sys_nice() */
#define SYS_PRIVCTL		(KERNEL_CALL + 4)	/* sys_privctl() */
#define SYS_TRACE		(KERNEL_CALL + 5)	/* sys_trace() */
#define SYS_KILL		(KERNEL_CALL + 6)	/* sys_kill() */

#define SYS_GETKSIG		(KERNEL_CALL + 7)	/* sys_getsig() */
#define SYS_ENDKSIG		(KERNEL_CALL + 8)	/* sys_endsig() */
#define SYS_SIGSEND		(KERNEL_CALL + 9)	/* sys_sigsend() */
#define SYS_SIGRETURN	(KERNEL_CALL + 10)	/* sys_sigreturn() */

#define SYS_NEWMAP		(KERNEL_CALL + 11)	/* sys_newmap() */
#define SYS_SEGCTL		(KERNEL_CALL + 12)	/* sys_segctl() */
#define SYS_MEMSET		(KERNEL_CALL + 13)	/* sys_memset() */

#define SYS_UMAP		(KERNEL_CALL + 14)	/* sys_umap() */
#define SYS_VIRCOPY		(KERNEL_CALL + 15)	/* sys_vircopy() */
#define SYS_PHYSCOPY	(KERNEL_CALL + 16)	/* sys_physcopy() */
#define SYS_VIRVCOPY	(KERNEL_CALL + 17)	/* sys_virvcopy() */
#define SYS_PHYSVCOPY	(KERNEL_CALL + 18)	/* sys_physvcopy() */

#define SYS_IRQCTL		(KERNEL_CALL + 19)	/* sys_irqctl() */
#define SYS_INT86		(KERNEL_CALL + 20)	/* sys_int86() */
#define SYS_DEVIO		(KERNEL_CALL + 21)	/* sys_devio() */
#define SYS_SDEVIO		(KERNEL_CALL + 22)	/* sys_sdevio() */
#define SYS_VDEVIO		(KERNEL_CALL + 23)	/* sys_vdevio() */

#define SYS_SETALARM	(KERNEL_CALL + 24)	/* sys_setalarm() */
#define SYS_TIMES		(KERNEL_CALL + 25)	/* sys_times() */
#define SYS_GETINFO		(KERNEL_CALL + 26)	/* sys_getinfo() */
#define SYS_ABORT		(KERNEL_CALL + 27)	/* sys_abort() */ 

#define NR_SYS_CALLS	28					/* Number of system calls */

#endif
