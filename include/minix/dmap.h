#ifndef _DMAP_H
#define _DMAP_H

#include "minix/sys_config.h"
#include "minix/ipc.h"

/*=====================================================================*
 *                Device <-> Driver Table                *
 *=====================================================================*/ 

/* Device table. This table is indexed by major device number. It provides
 * the link between major device numbers and the routines that process them.
 * The table can be update dynamically. The field 'dmap_flags' describe an
 * entry's current status and determines what control operations are possible.
 */
#define DMAP_MUTABLE	0x01	/* Mapping can be overtaken */
#define DMAP_BUSY		0x02	/* Driver busy with request */

enum DevStyle { STYLE_DEV, STYLE_NDEV, STYLE_TTY, STYLE_CLONE };

typedef struct {
	int (*dmap_opcl)(int, dev_t, int, int);		/* Open or close */
	void (*dmap_io)(int, Message *);
	int dmap_driver;
	int dmap_flags;
} DMap;

extern DMap dmapTable[];

/*=====================================================================*
 *                Major and minor device numbers                *
 *=====================================================================*/

/* Total number of different devices. */
#define NR_DEVICES			32	/* Number of (major) devices */

/* Major and minor device numbers for MEMORY driver. */
#define MEMORY_MAJOR		1	/* Major device for memory devices */
#define   RAM_DEV			0	/* Minor device for /dev/ram */
#define   MEM_DEV			1	/* Minor device for /dev/mem */
#define   KMEM_DEV			2	/* Minor device for /dev/kmem */
#define   NULL_DEV			3	/* Minor device for /dev/null */
#define   BOOT_DEV			4	/* Minor device for /dev/boot */
#define   ZERO_DEV			5	/* Minor device for /dev/zero */

#define CTRLR(n)	((n) == 0 ? 3 : (8 + 2 * ((n) - 1)))	/* Magic formula. (See initDMap in dmap.c) */

/* Full device numbers that are special to the boot monitor and FS. */
#define	DEV_RAM		0x0100 	/* Device number of /dev/ram, ((MEMORY_MAJOR << 8) | RAM_DEV) */
#define DEV_BOOT	0x0104	/* Device number of /dev/boot, ((MEMORY_MAJOR << 8) | BOOT_DEV) */

#define FLOPPY_MAJOR		2	/* Major device for floppy disks */
#define TTY_MAJOR			4	/* Major device for ttys */
#define CTTY_MAJOR			5	/* Major device for /dev/tty */

#define LOG_MAJOR			15	/* Major device for log driver */

#endif
