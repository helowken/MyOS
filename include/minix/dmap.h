#ifndef _DMAP_H
#define _DMAP_H

#include "minix/sys_config.h"
#include "minix/ipc.h"


/*=====================================================================*
 *                Major and minor device numbers                *
 *=====================================================================*/

/* Total number of different devices. */
#define NR_DEVICES			32		/* Number of (major) devices */

/* Major and minor device numbers for MEMORY driver. */
#define RAM_DEV				0	/* Minor device for /dev/ram */
#define MEM_DEV				1	/* Minor device for /dev/mem */
#define KMEM_DEV			2	/* Minor device for /dev/kmem */
#define NULL_DEV			3	/* Minor device for /dev/null */
#define BOOT_DEV			4	/* Minor device for /dev/boot */
#define ZERO_DEV			5	/* Minor device for /dev/zero */

/* Full device numbers that are special to the boot monitor and FS. */
#define	DEV_RAM			0x0100	/* Device number of /dev/ram */
#define DEV_BOOT		0x0104	/* Device number of /dev/boot */

#endif
