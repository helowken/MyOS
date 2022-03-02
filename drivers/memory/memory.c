/* This file contains the device dependent part of the drivers for the
 * following special files:
 *		/dev/ram		- RAM disk
 *		/dev/mem		- absolute memory
 *		/dev/kmem		- kernel virtual memory
 *		/dev/null		- null device (data sink)
 *		/dev/boot		- boot device loaded from boot image
 *		/dev/zero		- null byte stream generator
 */

#include "../drivers.h"
#include "../libdriver/driver.h"
#include "sys/ioc_memory.h"
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"

#define	NR_DEVS		6	/* Number of minor devices */

static Device mGeoms[NR_DEVS];	/* Base and size of each device */
static int mSegSels[NR_DEVS];		/* Segment index of each device */
static int mCurrDev;			/* Current device */
static KernelInfo kInfo;	/* Kernel information */
static Machine machine;			/* Machine information */

extern int errno;		/* Error number for PM calls */

static char *mName();
static Device *mPrepare(int device);
static int mTransfer(int pNum, int opCode, off_t pos, IOVec *iov, unsigned numReq);
static int mDoOpen(Driver *dp, Message *msg);
static int mIoctl(Driver *dp, Message *msg);
static void mGeometry(Partition *entry);

/* Entry points to this driver. */
static Driver mDriverTable = {
	mName,		/* Current device's name */
	mDoOpen,	/* Open or mount */
	doNop,		/* Nothing on a close */
	mIoctl,		/* Specify ram disk geometry */
	mPrepare,	/* Prepare for I/O on a given minor device */
	mTransfer,	/* Do the I/O */
	nopCleanup,	/* No need to clean up */
	mGeometry,	/* Memory device "geometry" */
	nopSignal,	/* System signals */
	nopAlarm,
	nopCancel,
	nopSelect,
	NULL,
	NULL
};

/* Buffer for the /dev/zero null byte feed. */
#define ZERO_BUF_SIZE	1024
static char devZero[ZERO_BUF_SIZE];

static void mInit() {
/* Initialize this task. All minor devices are initialized one by one. */
	int i, s;

	if ((s = sysGetKernelInfo(&kInfo)) != OK)
	  panic("MEM", "Couldn't get kernel information.", s);

	/* Install remote segment for /dev/kmem memory. */
	mGeoms[KMEM_DEV].dv_base = cvul64(kInfo.kernelMemBase);
	mGeoms[KMEM_DEV].dv_size = cvul64(kInfo.kernelMemSize);
	if ((s = sysSegCtl(&mSegSels[KMEM_DEV], (u16_t *) &s, (vir_bytes *) &s,
				kInfo.kernelMemBase, kInfo.kernelMemSize)) != OK) {
		panic("MEM", "Couldn't install remote segment.", s);
	}

	/* Install remote segment for /dev/boot memory, if enabled. */
	mGeoms[BOOT_DEV].dv_base = cvul64(kInfo.bootDevBase);
	mGeoms[BOOT_DEV].dv_size = cvul64(kInfo.bootDevSize);
	if (kInfo.bootDevBase > 0) {
		if ((s = sysSegCtl(&mSegSels[BOOT_DEV], (u16_t *) &s, (vir_bytes *) &s,
					kInfo.bootDevBase, kInfo.bootDevSize)) != OK) {
			panic("MEM", "Couldn't install remote segment", s);	
		}
	}

	/* Initialize /dev/zero. Simply write zeros into the buffer. */
	for (i = 0; i < ZERO_BUF_SIZE; ++i) {
		devZero[i] = 0;
	}

	/* Set up memory ranges for /dev/mem. */
	if ((s = sysGetMachine(&machine)) != OK)
	  panic("MEM", "Couldn't get machine information.", s);
	mGeoms[MEM_DEV].dv_size = cvul64(0xFFFFFFFF);	/* 4GB - 1 for 386 systems */
}

int main() {
	mInit();
	driverTask(&mDriverTable);
	return OK;
}

static char *mName() {
	static char name[] = "memory";
	return name;
}

static int mDoOpen(Driver *dp, Message *msg) {
/* Check device number on open. (This used to give I/O privileges to a
 * process opening /dev/mem or /dev/kmem. This may be needed in case of
 * memory mapped I/O. With system calls to do I/O this is no longer needed.)
 */
	if (mPrepare(msg->DEVICE) == NIL_DEV)
	  return ENXIO;
	return OK;
}

static Device *mPrepare(int device) {
/* Prepare for I/O on a device: check if the minor device number is ok. */
	if (device < 0 || device >= NR_DEVICES)
	  return NIL_DEV;
	mCurrDev = device;
	return &mGeoms[device];
}

static void mGeometry(Partition *entry) {
/* Memory devices don't have a geometry, but the outside world insists. */
#define HEADS	64
#define SECTORS 32
	entry->cylinders = div64u(mGeoms[mCurrDev].dv_size, SECTOR_SIZE) / 
							(HEADS * SECTORS);
	entry->heads = HEADS;
	entry->sectors = SECTORS;
}

static int mTransfer(int pNum, int opCode, off_t pos, 
			IOVec *iov, unsigned numReq) {
/* Read or write one of the driver's minor devices. */
	phys_bytes physAddr;
	int segSel;
	unsigned count, left, chunk;
	vir_bytes userAddr;
	Device *dv;
	unsigned long dvSize;
	int s;

	/* Get minor device number and check for /dev/null. */
	dv = &mGeoms[mCurrDev];	
	dvSize = cv64ul(dv->dv_size);

	while (numReq > 0) {
		/* How much to transfer and where to / from. */
		count = iov->iov_size;
		userAddr = iov->iov_addr;

		switch (mCurrDev) {
			/* No copying; ignore request. */
			case NULL_DEV:
				if (opCode == DEV_GATHER)
				  return OK;	/* Always at EOF */
				break;

			/* Virtual copying. For RAM disk, kernel memory and 
			 * boot device.
			 */
			case RAM_DEV:
			case KMEM_DEV:
			case BOOT_DEV:
				if (pos >= dvSize)
				  return OK;	/* Check for EOF */
				if (pos + count > dvSize)
				  count = dvSize - pos;
				segSel = mSegSels[mCurrDev];

				if (opCode == DEV_GATHER) 	/* Copy actual data */
				  sysVirCopy(SELF, segSel, pos, 
							  pNum, D, userAddr, count);	
				else 
				  sysVirCopy(pNum, D, userAddr, 
							  SELF, segSel, pos, count);
				break;

			/* Physical copying. Only used to access entire memory. */
			case MEM_DEV:
				if (pos >= dvSize)
				  return OK;	/* Check for EOF */
				if (pos + count > dvSize)
				  count = dvSize - pos;
				physAddr = cv64ul(dv->dv_base) + pos;

				if (opCode == DEV_GATHER)	/* Copy data */
				  sysPhysCopy(NONE, PHYS_SEG, physAddr, 
							  pNum, D, userAddr, count);
				else 
				  sysPhysCopy(pNum, D, userAddr, 
							  NONE, PHYS_SEG, physAddr, count);
				break;

			/* Null byte stream generator. */
			case ZERO_DEV:
				if (opCode == DEV_GATHER) {
					left = count;
					while (left > 0) {
						chunk = (left > ZERO_BUF_SIZE) ? ZERO_BUF_SIZE : left;
						if ((s = sysVirCopy(SELF, D, (vir_bytes) devZero,
											pNum, D, userAddr, chunk)) != OK)
						  report("MEM", "sysVirCopy failed", s);
						left -= chunk;
						userAddr += chunk;
					}
				}
				break;
			
			/* Unknown (illegal) minor device. */
			default:
				return EINVAL;
		}
	
		/* Book the number of bytes transferred. */
		pos += count;
		iov->iov_addr += count;
		if ((iov->iov_size -= count) == 0) {
			++iov;
			--numReq;
		}
	}
	return OK;
}

static int mIoctl(Driver *dp, Message *msg) {
/* I/O controls for the memory driver. Currently there is one I/O control:
 * - MEM_IOC_RAM_SIZE: to set the size of the RAM disk.
 */
	Device *dv;
	phys_bytes ramDevBase;
	phys_bytes ramDevSize;
	int s;

	if ((dv = mPrepare(msg->DEVICE)) == NIL_DEV)
	  return ENXIO;

	switch (msg->REQUEST) {
		/* FS wants to create a new RAM disk with the given size. */
		case MEM_IOC_RAM_SIZE:
			if (msg->PROC_NR != FS_PROC_NR) {
				report("MEM", "warning, MEM_IOC_RAM_SIZE called by", msg->PROC_NR);
				return EPERM;
			}

			/*Try to allocate a piece of memory for the RAM disk. */
			ramDevSize = msg->POSITION;
			if (allocMem(ramDevSize, &ramDevBase) < 0) {
				report("MEM", "warning, allocMem failed", errno);
				return ENOMEM;
			}
			dv->dv_base = cvul64(ramDevBase);
			dv->dv_size = cvul64(ramDevSize);

			if ((s = sysSegCtl(&mSegSels[RAM_DEV], (u16_t *) &s, (vir_bytes *) &s,
							ramDevBase, ramDevSize)) != OK)
			  panic("MEM", "Couldn't install remote segment.", s);
			break;
		
		default:
			return doDrIoctl(&mDriverTable, msg);
	}
	return OK;
}


