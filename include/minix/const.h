#ifndef _MINIX_CONST_H
#define _MINIX_CONST_H

#define EXTERN		extern		/* Used in *.h files */

#define HZ				60		/* clock freq (software settable on IBM-PC) */

#define SUPER_USER		0		/* uid_t of superuser */

#define NR_REMOTE_SEGS	3		/* Remote memory regions */

/* Memory related constants. */
#define SEGMENT_TYPE	0xFF00	/* Bit mask to get segment type */
#define SEGMENT_INDEX	0x00FF	/* Bit mask to get segment index */

#define LOCAL_SEG		0x0000	/* Flags indicating local memory segment */
#define NR_LOCAL_SEGS	3		/* Local segments per process (fixed) */
#define T				0		/* proc[i].p_memmap[T] is for text */
#define D				1		/* proc[i].p_memmap[D] is for data */
#define S				2		/* proc[i].p_memmap[S] is for stack */

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
#define BYTE			0377	/* Mark for 8 bits */
#define	NO_NUM			0x8000	/* Used as numerical argument to panic() */

#define CLICK_SIZE		1024	/* Unit in which memory is allocated */
#define CLICK_SHIFT		10		/* log2 of CLICK_SIZE */

/* Flag bits for i_node in the INode. */
#define I_TYPE			0170000	/* This field gives inode type */
#define I_REGULAR		0100000	/* Regular file, not dir or special */
#define I_BLOCK_SPECIAL 0060000 /* Block special file */
#define I_DIRECTORY		0040000 /* File is a directory */
#define I_CHAR_SPECIAL	0020000	/* Character special file */
#define I_NAMED_PIPE	0010000	/* Named pipe (FIFO) */
#define I_SET_UID_BIT	0004000	/* Set effective uid_t on exec */
#define I_SET_GID_BIT	0002000	/* Set effective gid_t on exec */
#define ALL_MODES		0006777	/* All bits for user, group, and others */
#define RWX_MODES		0000777	/* Mode bits for RWX only */
#define R_BIT			0000004	/* Rwx protection bit */
#define W_BIT			0000002	/* rWx protection bit */
#define X_BIT			0000001	/* rwX protection bit */
#define I_NOT_ALLOC		0000000	/* This inode is free */

#endif
