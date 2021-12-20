#ifndef _MINIX_CONST_H
#define _MINIX_CONST_H

#define EXTERN		extern	/* Used in *.h files */

#define HZ				60	/* clock freq (software settable on IBM-PC) */

#define SUPER_USER		0	/* uid_t of superuser */

#define NR_REMOTE_SEGS	3	/* Remote memory regions */

/* Memory related constants. */
#define SEGMENT_TYPE	0xFF00	/* Bit mask to get segment type */
#define SEGMENT_INDEX	0x00FF	/* Bit mask to get segment index */

#define LOCAL_SEG		0x0000	/* Flags indicating local memory segment */
#define NR_LOCAL_SEGS	3	/* Local segments per process (fixed) */
#define T				0	/* proc[i].p_memmap[T] is for text */
#define D				1	/* proc[i].p_memmap[D] is for data */
#define S				2	/* proc[i].p_memmap[S] is for stack */

#define REMOTE_SEG		0x0100	/* Flags indicating remote memory segment */
#define NR_REMOTE_SEGS	3		/* # Remote memory regions (variable) */

#define BIOS_SEG		0x0200	/* Flags indicating BIOS memory segment */
#define NR_BIOS_SEGS	3		/* # BIOS memory regions (variable) */

#define PHYS_SEG		0x0400	/* Flags indicating entire physical memory */

/* Macros. */
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define MIN(a, b)		((a) < (b) ? (a) : (b))

/* Process name length in the PM process table, including '\0'. */
#define PROC_NAME_LEN	16

/* Miscellaneous */
#define BYTE		0377	/* Mark for 8 bits */
#define	NO_NUM		0x8000	/* Used as numerical argument to panic() */

#define CLICK_SIZE		1024	/* Unit in which memory is allocated */
#define CLICK_SHIFT		10		/* log2 of CLICK_SIZE */

#endif
