#ifndef	_PROTECT_H
#define _PROTECT_H

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


/* Descriptor structure offsets. */
#define DESC_SIZE			8		/*	sizeof SegDesc */

#endif
