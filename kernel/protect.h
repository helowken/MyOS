#ifndef	PROTECT_H
#define PROTECT_H

/* Table sizes. */
#define GDT_SIZE		(FIRST_LDT_INDEX + NR_TASKS + NR_PROCS)		

/* Fixed global descriptors. 1 to 7 are prescribed by the BIOS. */
#define GDT_INDEX			1		/* GDT descriptor */
#define IDT_IDNEX			2		/* IDT descriptor */
#define DS_INDEX			3		/* Kernel DS */
#define ES_INDEX			4		/* Kernel ES (flat 4 GB at startup) */
#define SS_INDEX			5		/* Kernel SS (monitor SS at startup) */
#define CS_INDEX			6		/* Kernel CS */
#define MON_CS_INDEX		7		/* Temp for BIOS (monitor CS at startup) */

#define FIRST_LDT_INDEX		15		/* Rest of descriptors are LDT's */

#define GDT_SELECTOR		0x08	/* (GDT_INDEX * DESC_SIZE) */
#define IDT_SELECTOR		0x10	/* (IDT_INDEX * DESC_SIZE) */
#define DS_SELECTOR			0x18	/* (DS_INDEX * DESC_SIZE) */
#define ES_SELECTOR			0x20	/* (ES_INDEX * DESC_SIZE) */
#define	SS_SELECTOR			0x28	/* (SS_INDEX * DESC_SIZE) */
#define CS_SELECTOR			0x30	/* (CS_INDEX * DESC_SIZE) */
#define MON_CS_SELECTOR		0x38	/* (MON_CS_INDEX * DESC_SIZE) */

/* Privileges. */
#define INTR_PRIVILEGE		0		/* Kernel and interrupt handlers. */
#define TASK_PRIVILEGE		1		/* Kernel tasks. */
#define USER_PRIVILEGE		2		/* Servers and user processes. */

/* 
 * Exception vector numbers.
 *
 * Type: A(bort) / F(ault) / I(nterrupt) / T(rap) 
 * Error Code (EC): Y(es) / N(o)
 */
									/* Description              | Type | EC */
#define BOUNDS_VECTOR		5		/* BOUND Range Exceeded     |    F | N  */
#define INVAL_OP_VECTOR		6		/* Invalid Opcode           |    F | N          
									   (Undefined Opcode)                   */
#define COPROC_NOT_VECTOR	7		/* Device Not Available     |    F | N
									   (No Math Coprocessor)                */
#define DOUBLE_FAULT_VECTOR	8		/* Double Fault             |    A | Y  */
#define COPROC_SEG_VECTOR	9		/* Coprocessor Segment      |    F | N
									   Overrun (reserved)                   */
#define INVAL_TSS_VECTOR	10		/* Invalid TSS              |    F | Y  */
#define SEG_NOT_VECTOR		11		/* Segment Not Present      |    F | Y  */
#define STACK_FAULT_VECTOR	12		/* Stack-Segment Fault      |    F | Y  */
#define PROTECTION_VECTOR	13		/* General Protection       |    F | Y  */
#define PAGE_FAULT_VECTOR	14		/* Page Fault               |    F | Y  */
/*							15		   Intel reserved. Do not use.          */
#define COPROC_ERR_VECTOR	16		/* x87 FPU Floating-Point   |    F | N  
									   Error (Math Fault)                   */


/* Descriptor structure offsets. */
#define DESC_SIZE			8		/*	sizeof SegDesc */

#endif
