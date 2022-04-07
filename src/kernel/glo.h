#ifndef	GLO_H
#define	GLO_H

#ifdef	_TABLE
#undef	EXTERN
#define	EXTERN
#endif

#include "minix/config.h"
#include "config.h"
#include "proc.h"

EXTERN char shutdownStarted;	/* TRUE after shutdowns / reboots */

/* Kernel information structures. This groups vital kernel information. */
EXTERN phys_bytes imgHdrPos;	/* Address of image headers */
EXTERN KernelInfo kernelInfo;	/* Kernel information for users */
EXTERN Machine machine;			/* Machine information for users */
EXTERN KernelMessages kernelMessages;	/* Diagnostic messages in kernel */
EXTERN Randomness kernelRandom;	/* Gather kernel random information */

/* Process scheduling information and the kernel reentry count. */
EXTERN Proc *prevProc;
EXTERN Proc *currProc;
EXTERN Proc *nextProc;
EXTERN Proc *billProc;
EXTERN char kernelReentryCount;	/* Kernel reentry count (entry count less 1) */
EXTERN unsigned lostTicks;		/* Clock ticks counted outside clock task */

/* Interrupt related variables. */
EXTERN IrqHook irqHooks[NR_IRQ_HOOKS];		/* Hooks for general use */
EXTERN IrqHook *irqHandlers[NR_IRQ_VECTORS];	/* List of IRQ handlers */
EXTERN int irqActiveIds[NR_IRQ_VECTORS];	/* IRQ ID bits active */
EXTERN int irqInUse;			/* Map of all in-use irq's */

EXTERN void (*level0Func)();

/* Variables that are initialized elsewhere are just extern here. */
extern BootImage images[];
extern char *taskStack[];		/* Task stack space */
extern SegDesc gdt[];			/* GDT (global descriptor table) */

#endif
