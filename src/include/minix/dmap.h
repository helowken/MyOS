#ifndef _DMAP_H
#define _DMAP_H

#include <minix/sys_config.h>
#include <minix/ipc.h>
#include <minix/dmap_nr.h>

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
	int (*dmap_opcl)(int op, dev_t dev, int proc, int flag);	/* Open or close */
	void (*dmap_io)(int driver, Message *msg);
	int dmap_driver;
	int dmap_flags;
} DMap;

extern DMap dmapTable[];

#endif
