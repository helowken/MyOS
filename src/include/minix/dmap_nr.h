#ifndef _DMAP_NR_H
#define _DMAP_NR_H

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
#define	DEV_RAM			0x0100 	/* Device number of /dev/ram, ((MEMORY_MAJOR << 8) | RAM_DEV) */
#define DEV_BOOT		0x0104	/* Device number of /dev/boot, ((MEMORY_MAJOR << 8) | BOOT_DEV) */

#define FLOPPY_MAJOR		2	/* Major device for floppy disks */
#define TTY_MAJOR			4	/* Major device for ttys */
#define CTTY_MAJOR			5	/* Major device for /dev/tty */
#define INET_MAJOR			7	/* Major device for inet */
#define LOG_MAJOR			15	/* Major device for log driver */
#define   IS_KLOG_DEV		0	/* Minor device for /dev/klog */

#endif
