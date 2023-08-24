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
#define NR_BOOT_PROCS	12	// TODO (NR_TASKS + INIT_PROC_NR + 1)

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
#define NOTIFY_FROM(pNum)	(NOTIFY_MESSAGE | ((pNum) + NR_TASKS))
#	define SYN_ALARM	NOTIFY_FROM(CLOCK)		/* Synchronous alarm */
#	define SYS_SIG		NOTIFY_FROM(SYSTEM)		/* System signal */
#	define HARD_INT		NOTIFY_FROM(HARDWARE)	/* Hardware interrupt */
#	define NEW_KSIG		NOTIFY_FROM(HARDWARE)	/* New kernel signal */
#	define FKEY_PRESSED		NOTIFY_FROM(TTY_PROC_NR)	/* Function key press */

/* Shorthands for message parameters passed with notifications. */
#define NOTIFY_SOURCE	m_source
#define NOTIFY_TYPE		m_type
#define NOTIFY_ARG		m2_l1
#define NOTIFY_TIMESTAMP	m2_l2
#define NOTIFY_FLAGS		m2_i1

/*=====================================================================*
 *			Messages for BLOCK and CHARACTER device drivers			*
 *=====================================================================*/

/* Message types for device drivers. */
#define DEV_REQ_BASE	0x400	/* Base for device request types */
#define DEV_RES_BASE	0x500	/* Base for device response types */

#define CANCEL			(DEV_REQ_BASE + 0)	/* General req to force a task to cancel */
#define DEV_READ		(DEV_REQ_BASE + 3)	/* Read from minor device */
#define DEV_WRITE		(DEV_REQ_BASE + 4)	/* Write to minor device */
#define DEV_IOCTL		(DEV_REQ_BASE + 5)	/* I/O control code */
#define DEV_OPEN		(DEV_REQ_BASE + 6)  /* Open a minor device */
#define DEV_CLOSE		(DEV_REQ_BASE + 7)  /* Close a minor device */
#define DEV_SCATTER		(DEV_REQ_BASE + 8)  /* Write from a vector */
#define DEV_GATHER		(DEV_REQ_BASE + 9)  /* Read into a vector */
#define TTY_SET_PRGP	(DEV_REQ_BASE + 10)	/* Set process group */
#define TTY_EXIT		(DEV_REQ_BASE + 11)	/* Process group leader exited */
#define DEV_SELECT		(DEV_REQ_BASE + 12)	/* Request select() attention */
#define DEV_STATUS		(DEV_REQ_BASE + 13)	/* Request driver status */

#define DEV_REPLY		(DEV_RES_BASE + 0)	/* General task reply */
#define DEV_CLONED		(DEV_RES_BASE + 1)	/* Return cloned minor */
#define DEV_REVIVE		(DEV_RES_BASE + 2)	/* Driver revives process */
#define DEV_IO_READY	(DEV_RES_BASE + 3)	/* Selected device ready */
#define DEV_NO_STATUS	(DEV_RES_BASE + 4)	/* Empty status reply */

/* Field names for messages to block and character device drivers. */
#define DEVICE			m2_i1	/* Major-minor device */
#define PROC_NR			m2_i2	/* Which (proc) wants I/O? */
#define COUNT			m2_i3	/* How many bytes to transfer */
#define REQUEST			m2_i3	/* Ioctl request code */
#define POSITION		m2_l1	/* File offset */
#define ADDRESS			m2_p1	/* Core buffer address */

/* Field names for DEV_SELECT messages to device drivers. */
#define DEV_MINOR		m2_i1	/* Minor device */
#define DEV_SEL_OPS		m2_i2	/* Which select operations are requested */
#define DEV_SEL_WATCH	m2_i3	/* Request notify if no operations are ready */

/* Field names used in reply messages from tasks. */
#define REP_PROC_NR		m2_i1	/* # of proc on whose behalf I/O was done */
#define REP_STATUS		m2_i2	/* Bytes transferred or error number */
#	define SUSPEND		-998	/* Status to suspend caller, reply later */

/* Field names for messages to TTY driver. */
#define TTY_LINE		DEVICE	/* Message parameter: terminal line */
#define TTY_REQUEST		COUNT	/* Message parameter: ioctl request code */
#define TTY_SPEK		POSITION/* Message parameter: ioctl speed, erasing */
#define TTY_FLAGS		m2_l2	/* Message parameter: ioctl tty mode */
#define TTY_PGRP		m2_i3	/* Message parameter: process group */

/*=====================================================================*
 *			SYSTASK	request types and field names			*
 *=====================================================================*/

/* System library calls are dispatched via a call vector. 
 * The numbers here determine which call is made from the call vector.*/
#define KERNEL_CALL	0x600	/* Base for kernel calls to SYSTEM */

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

#define NR_SYS_CALLS	28		/* Number of system calls */

/* Field names for SYS_MEMSET, SYS_SEGCTL. */
#define MEM_PTR			m2_p1	/* Base */
#define MEM_COUNT		m2_l1	/* Count */
#define MEM_PATTERN		m2_l2	/* Pattern to write */

/* Field names for SYS_DEVIO, SYS_VDEVIO, SYS_SDEVIO. */
#define DIO_REQUEST		m2_i3	/* Device in or output */
#	define DIO_INPUT	0		/* Input */
#	define DIO_OUTPUT	1		/* Output */
#define DIO_TYPE		m2_i1	/* Flag indicating byte, word, or long */
#	define DIO_BYTE		'b'		/* Byte type values */
#	define DIO_WORD		'w'		/* Word type values */
#	define DIO_LONG		'l'		/* Long type values */
#define DIO_PORT		m2_l1	/* Single port address */
#define DIO_VALUE		m2_l2	/* Single I/O value */
#define DIO_VEC_ADDR	m2_p1	/* Address of buffer or (p,v)-pairs */
#define	DIO_VEC_SIZE	m2_l2	/* Number of elements in vector */
#define DIO_VEC_PROC	m2_i2	/* Number of process where vector is */

/* Field names for SYS_SETALARM. */
#define ALARM_EXP_TIME	m2_l1	/* Expire time for the alarm call */
#define ALARM_ABS_TIME	m2_i2	/* Set to 1 to use absolute alarm time */
#define ALARM_TIME_LEFT	m2_l1	/* How many ticks were remaining */
#define ALARM_FLAG_PTR	m2_p1	/* Virtual address of timeout flag */

/* Field names for SYS_ABORT. */
#define ABORT_HOW		m1_i1	/* RBT_REBOOT, RBT_HALT, etc. */
#define ABORT_MON_PROC	m1_i2	/* Process where monitor params are */
#define ABORT_MON_LEN	m1_i3	/* Length of monitor params */
#define ABORT_MON_ADDR	m1_p1	/* Virtual address of monitor params */

/* Field names for UMAP, VIRCOPY, PHYSCOPY. */
#define CP_SRC_SPACE	m5_c1	/* T or D space (stack is also D) */
#define CP_SRC_PROC_NR	m5_i1	/* Process to copy from */
#define CP_SRC_ADDR		m5_l1	/* Address where data come from */
#define CP_DST_SPACE	m5_c2	/* T or D space (stack is alos D) */
#define CP_DST_PROC_NR	m5_i2	/* Process is copy to */
#define CP_DST_ADDR		m5_l2	/* Address where data go to */
#define CP_NR_BYTES		m5_l3	/* Number of bytes to copy */

/* Field names for SYS_GETINFO. */
#define I_REQUEST		m7_i3	/* What info to get */
#	define GET_KINFO		0	/* Get kernel information structure */
#	define GET_IMAGE		1	/* Get system image table */
#	define GET_PROCTAB		2	/* Get kernel process table */
#	define GET_RANDOMNESS	3	/* Get randomness buffer */
#	define GET_MONPARAMS	4	/* Get monitor parameters */
#	define GET_KENV			5	/* Get Kernel environment string */
#	define GET_IRQHOOKS		6	/* Get the IRQ table */
#	define GET_KMESSAGES	7	/* Get kernel messages */
#	define GET_PRIVTAB		8	/* Get kernel privileges table */
#	define GET_KADDRESSES	9	/* Get various kernel addresses */
#	define GET_SCHEDINFO	10	/* Get scheduling queues */
#	define GET_PROC			11	/* Get process slot if given process */
#	define GET_MACHINE		12	/* Get machine information */
#	define GET_LOCKTIMING	13	/* Get lock()/unlock() latency timing */
#	define GET_BIOSBUFFER	14	/* Get a buffer for BIOS calls */
#define	I_PROC_NR		m7_i4	/* Calling process */
#define I_VAL_PTR		m7_p1	/* Virtual address at caller */
#define I_VAL_LEN		m7_i1	/* Max length of value */
#define	I_VAL_PTR2		m7_p2	/* Second virtual address */
#define I_VAL_LEN2		m7_i2	/* Second length, or proc num */

/* Field names for SYS_TIMES. */
#define T_PROC_NR		m4_l1	/* Process to request time info for */
#define T_USER_TIME		m4_l1	/* User time consumed by prcoess */
#define T_SYSTEM_TIME	m4_l2	/* System time consumed by process */
#define T_BOOT_TICKS	m4_l5	/* Number of clock ticks since boot time */

/* Field names for SYS_TRACE, SYS_SVRCTL. */
#define CTL_PROC_NR		m2_i1	/* Process number of the caller */
#define CTL_REQUEST		m2_i2	/* Server control request */
#define CTL_MM_PRIV		m2_i3	/* Privilege as seen by PM */
#define CTL_ARG_PTR		m2_p1	/* Pointer to argument */
#define	CTL_ADDRESS		m2_l1	/* Address at traced process' space */
#define CTL_DATA		m2_l2	/* Data field for tracing */

/* Field names for SYS_KILL, SYS_SIGCTL. */
#define SIG_REQUEST		m2_l2	/* PM signal control request */
#define S_GETSIG		0		/* Get pending kernel signal */
#define S_ENDSIG		1		/* Finish a kernel signal */
#define S_SENDSIG		2		/* POSIX style signal handling */
#define S_SIGRETURN		3		/* Return from POSIX handling */
#define S_KILL			4		/* Server kills process with signal */
#define	SIG_PROC		m2_i1	/* Process number for inform */
#define SIG_NUMBER		m2_i2	/* Signal number to send */
#define SIG_FLAGS		m2_i3	/* Signal flags field */
#define SIG_MAP			m2_l1	/* Used by kernel to pass signal bit map */
#define SIG_CTXT_PTR	m2_p1	/* Poitner to info to restore signal context */

/* Field names for SYS_FORK, EXEC, EXIT, NEWMAP. */
#define PR_PROC_NR		m1_i1	/* Indicates a (child) process */
#define PR_PRIORITY		m1_i2	/* Process priority */
#define PR_PPROC_NR		m1_i2	/* Indicates a (parent) process */
#define PR_PID			m1_i3	/* Process id at process manager */
#define PR_STACK_PTR	m1_p1	/* Used for stack ptr */
#define PR_TRACING		m1_i3	/* Flag to indicate tracing is on / off */
#define PR_NAME_PTR		m1_p2	/* Tells where program name is for dmp */
#define PR_IP_PTR		m1_p3	/* Initial value for ip after exec */
#define	PR_MEM_PTR		m1_p1	/* Tells where memory map */

/* Field names for SYS_IRQCTL. */
#define IRQ_REQUEST		m5_c1	/* What to do? */
#  define IRQ_SET_POLICY	1	/* Manage a slot of the IRQ table */
#  define IRQ_RM_POLICY		2	/* Remove a slot of the IRQ table */
#  define IRQ_ENABLE		3	/* Enable interrupts */
#  define IRQ_DISABLE		4	/* Disable interrupts */
#define IRQ_VECTOR		m5_c2	/* Irq vector */
#define IRQ_POLICY		m5_i1	/* Options for IRQCTL request */
#  define IRQ_REENABLE	0x001	/* Reenable IRQ line after interrupt */
#  define IRQ_BYTE		0x100	/* Byte values */
#  define IRQ_WORD		0x200	/* Word values */
#  define IRQ_LONG		0x400	/* Long values */
#define IRQ_PROC_NR		m5_i2	/* Process number, SELF, NONE */
#define	IRQ_HOOK_ID		m5_l3	/* Id of irq hook at kernel */

/* Field names for SYS_SEGCTL. */
#define SEG_SELECT		m4_l1	/* Segment selector returned */
#define SEG_OFFSET		m4_l2	/* Offset in segment returned */
#define SEG_PHYS_ADDR	m4_l3	/* Physical address of segment */
#define SEG_SIZE		m4_l4	/* Segment size */
#define SEG_INDEX		m4_l5	/* Segment index in remote map */

/* Field names for SELECT (FS). */
#define SEL_NFDS		m8_i1
#define SEL_READFDS		m8_p1
#define SEL_WRITEFDS	m8_p2
#define SEL_ERRORFDS	m8_p3
#define SEL_TIMEOUT		m8_p4

/*=====================================================================*
 *			Messages for system management server   	  *
 *=====================================================================*/
#define SRV_REQ_BASE	0x700

#define SRV_UP			(SRV_REQ_BASE + 0)	/* Start system service */
#define SRV_DOWN		(SRV_REQ_BASE + 1)	/* Stop system servie */
#define SRV_STATUS		(SRV_REQ_BASE + 2)	/* Get services status */

#  define SRV_PATH_ADDR		m1_p1	/* Path of binary */
#  define SRV_PATH_LEN		m1_i1	/* Length of binary */
#  define SRV_ARGS_ADDR		m1_p2	/* Arguments to be passed */
#  define SRV_ARGS_LEN		m1_i2	/* Length of arguments */
#  define SRV_DEV_MAJOR		m1_i3	/* Major device number */
#  define SRV_PRIV_ADDR		m1_p3	/* Privileges string */
#  define SRV_PRIV_LEN		m1_i3	/* Length of privileges */

/*=====================================================================*
 *			Miscellaneous messages used by TTY			*
 *=====================================================================*/

/* Miscellaneous request types and field names, e.g. used by IS server. */
#define PANIC_DUMPS			97	/* Debug dumps at the TTY on RBT_PANIC */
#define FKEY_CONTROL		98	/* Control a function key at the TTY */
#define	  FKEY_REQUEST	m2_i1	/* Request to perform at TTY */
#define	     FKEY_MAP		10	/* Observe function key */
#define	     FKEY_UNMAP		11	/* Stop observing function key */
#define      FKEY_EVENTS	12	/* Request open key presses */
#define   FKEY_FKEYS	m2_l1	/* F1-F12 keys pressed */
#define   FKEY_SFKEYS	m2_l2	/* Shift-F1-F12 keys pressed */
#define DIAGNOSTICS			100	/* Output a string without FS in between */
#define   DIAG_PRINT_BUF	m1_p1
#define	  DIAG_BUF_COUNT	m1_i1
#define   DIAG_PROC_NR		m1_i2

#endif
