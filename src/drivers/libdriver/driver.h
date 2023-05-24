#define _POSIX_SOURCE		1
#define _MINIX				1
#define _SYSTEM				1

#include <minix/config.h>
#include <minix/type.h>
#include <minix/ipc.h>
#include <minix/com.h>
#include <minix/callnr.h>
#include <sys/types.h>
#include <minix/const.h>
#include <minix/syslib.h>
#include <minix/sysutil.h>

#include <string.h>
#include <limits.h>
#include <stddef.h>
#include <errno.h>

#include <minix/partition.h>
#include <minix/u64.h>

#define CD_SECTOR_SIZE		2048	/* Sector size of a CD-ROM */

/* Base and size of a partition in bytes. */
typedef struct {
	u64_t dv_base;
	u64_t dv_size;
} Device;

/* Info about and entry points into the device dependent code. */
typedef struct Driver {
	char *(*drName)();
	int (*drOpen)(struct Driver *dp, Message *msg);
	int (*drClose)(struct Driver *dp, Message *msg);
	int (*drIoctl)(struct Driver *dp, Message *msg);
	Device *(*drPrepare)(int device);
	int (*drTransfer)(int pNum, int opCode, off_t position,
				IOVec *iov, unsigned numReq);
	void (*drCleanup)();
	void (*drGeometry)(Partition *entry);
	void (*drSignal)(struct Driver *dp, Message *msg);
	void (*drAlarm)(struct Driver *dp, Message *msg);
	int (*drCancel)(struct Driver *dp, Message *msg);
	int (*drSelect)(struct Driver *dp, Message *msg);
	int (*drOther)(struct Driver *dp, Message *msg);
	int (*drHwInt)(struct Driver *dp, Message *msg);
} Driver;

extern u8_t *tmpBuf;			/* The DMA buffer */
extern phys_bytes tmpPhysAddr;	/* Phys address of DMA buffer */

/* Functions defined by driver.c */
void driverTask(Driver *dr);
char *noName();
int doNop(Driver *dp, Message *msg);
Device *nopPrepare(int device);
void nopCleanup();
void nopSignal(Driver *dp, Message *msg);
void nopAlarm(Driver *dp, Message *msg);
int nopCancel(Driver *dp, Message *msg);
int nopSelect(Driver *dp, Message *msg);
int doDrIoctl(Driver *dp, Message *msg);

#define NIL_DEV			((Device *) 0)

/* Parameters for the disk drive. */
#define SECTOR_SIZE		512		/* Physical sector size in bytes */
#define SECTOR_SHIFT	9		/* For division */
#define SECTOR_MASK		511		/* and remainder */

/* Size of the DMA buffer in bytes. */
#define DMA_BUF_SIZE	(DMA_SECTORS * SECTOR_SIZE)

