#ifndef _MINIX_CONST_H
#define _MINIX_CONST_H

#define EXTERN		extern		/* Used in *.h files */

#define TRUE			1		/* Used for turning integers into Booleans */
#define FALSE			0		/* Used for turning integers into Booleans */

#define HZ				60		/* clock freq (software settable on IBM-PC) */

#define SUPER_USER		0		/* uid_t of superuser */

/* Devices. */
#define MAJOR			8		/* Major device = (dev >> MAJOR) & BYTE */
#define MINOR			0		/* Minor device = (dev >> MINOR) & BYTE */

#define MAJOR_DEV(dev)	(((dev) >> MAJOR) & BYTE)
#define MINOR_DEV(dev)	(((dev) >> MINOR) & BYTE)

#define NR_IO_REQS		MIN(NR_BUFS, 64)	/* Maximum number of entries in an I/O request */

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
#define	READING			0		/* Copy data to user */
#define WRITING			1		/* Cooy data from user */
#define	NO_NUM			0x8000	/* Used as numerical argument to panic() */
#define NIL_PTR		(char *) 0	/* Generally useful expression */

#define CLICK_SIZE		1024	/* Unit in which memory is allocated */
#define CLICK_SHIFT		10		/* log2 of CLICK_SIZE */
#define SIZE_TO_CLICKS(s)	(((s) + CLICK_SIZE - 1) >> CLICK_SHIFT)
#define CLICKS_TO_SIZE(s)	((s) << CLICK_SHIFT)

/* Click to byte conversions (and vice versa). */
#define HCLICK_SHIFT	4		/* log2 of HCLICK_SIZE */
#define HCLICK_SIZE		16		/* Hardware segment conversion magic */
#define hclickToPhys(n)	((phys_bytes) (n) << HCLICK_SHIFT)
#define physToHClick(n)	((n) >> HCLICK_SHIFT)

/* Flag bits for i_mode in the Inode. */
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

/* Flag used only in flags argument of devOpen. */
#define RO_BIT			0200000	/* Open device readonly; fail if writable. */

/* Some limits. */
#define MAX_BLOCK_NR	((block_t)  077777777)	/* Largest block number */
#define HIGHEST_ZONE	((zone_t)   077777777)	/* Largest zone number */
#define MAX_INODE_NR	((ino_)	 037777777777)	/* Largest inode number */
#define MAX_FILE_POS	((off_t) 037777777777)	/* Largest legal file offset */

#define NO_BLOCK		((block_t) 0)	/* Absence of a block number */
#define NO_ENTRY		((ino_t)   0)	/* Absence of a dir entry */
#define NO_ZONE			((zone_t)  0)	/* Absence of a zone number */
#define NO_DEV			((dev_t)   0)	/* Absence of a device number */

#endif
