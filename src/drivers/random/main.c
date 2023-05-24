#include "../drivers.h"
#include "../libdriver/driver.h"
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"

#include <assert.h>
#include "random.h"

#define NR_DEVS			1	/* Number of minor devices */
#define   RANDOM_DEV	0	/* Minor device for /dev/random */

#define KRANDOM_PERIOD	1	/* Ticks between krandom calls */

static Device rGeom[NR_DEVS];	/* Base and size of each device */
static int rCurrDev;		/* Current device */
extern int errno;			/* Errno number for PM calls */

/* Buffer for the /dev/random number generator. */
#define RANDOM_BUF_SIZE		1024
static char randomBuf[RANDOM_BUF_SIZE];

static int rIoctl(Driver *dp, Message *msg);

static char *rName() {
	static char name[] = "random";
	return name;
}

static Device *rPrepare(int device) {
	if (device < 0 || device >= NR_DEVS)
	  return NIL_DEV;
	rCurrDev = device;

	return &rGeom[device];
}

static int rDoOpen(Driver *dp, Message *msg) {
	if (rPrepare(msg->DEVICE) == NIL_DEV)
	  return ENXIO;
	return OK;
}

static void rGeometry(Partition *entry) {
	entry->cylinders = div64u(rGeom[rCurrDev].dv_size, SECTOR_SIZE) / (64 * 32);
	entry->heads = 64;
	entry->sectors = 32;
}

static void rRandom(Driver *dp, Message *msg) {
/* Fetch random information from the kernel to update /dev/random. */
	int i, s, rNext, rSize, rHigh;
	Randomness kRandom;

	if ((s = sysGetRandomness(&kRandom)) != OK)
	  report("RANDOM", "sysGetRandomness failed", s);

	for (i = 0; i < RANDOM_SOURCES; ++i) {
		rNext = kRandom.bin[i].r_next;
		rSize = kRandom.bin[i].r_size;
		rHigh = rNext + rSize;

		if (rHigh <= RANDOM_ELEMENTS) {
			randomUpdate(i, &kRandom.bin[i].r_buf[rNext], rSize);
		} else {
			assert(rNext < RANDOM_ELEMENTS);
			randomUpdate(i, &kRandom.bin[i].r_buf[rNext], RANDOM_ELEMENTS - rNext);
			randomUpdate(i, &kRandom.bin[i].r_buf[0], rHigh - RANDOM_ELEMENTS);
		}
	}

	/* Schedule new alarm for next rRandom call. */
	if ((s = sysSetAlarm(KRANDOM_PERIOD, 0)) != OK)
	  report("RANDOM", "sysSetAlarm failed", s);
}

static int rTransfer(
	int pNum,			/* Process doing the request */
	int opCode,			/* DEV_GATHER or DEV_SCATTER */
	off_t position,		/* Offset on device to read or write */
	IOVec *iov,			/* Pointer to read or write request vector */
	unsigned numReq		/* Length of request vector */
) {
/* Read or write one of the driver's minor devices. */
	unsigned count, left, chunk;
	vir_bytes userVir;

	if (rCurrDev != RANDOM_DEV)
	  return EINVAL;

	while (numReq > 0) {
		count = iov->iov_size;
		userVir = iov->iov_addr;

		if (opCode == DEV_GATHER && ! randomIsSeeded())
		  return EAGAIN;
		left = count;
		while (left > 0) {
			chunk = (left > RANDOM_BUF_SIZE) ? RANDOM_BUF_SIZE : left;
			if (opCode == DEV_GATHER) {
				randomGetBytes(randomBuf, chunk);
				sysVirCopy(SELF, D, (vir_bytes) randomBuf, 
							pNum, D, userVir, chunk);
			} else if (opCode == DEV_SCATTER) {
				sysVirCopy(pNum, D, userVir,
						SELF, D, (vir_bytes) randomBuf, chunk);
				randomPutBytes(randomBuf, chunk);
			}
			userVir += chunk;
			left -= chunk;
		}

		iov->iov_addr += count;
		if ((iov->iov_size -= count) == 0) {
			++iov;
			--numReq;
		}
	}
	return OK;
}

/* Entry points to this driver. */
static Driver rDriverTable = {
	rName,		/* Current device's name */
	rDoOpen,	/* Open or mount */
	doNop,		/* Nothing on a close */
	rIoctl,		/* Specify ram disk geometry */
	rPrepare,	/* Prepare for I/O on a given minor device */
	rTransfer,	/* Do the I/O */
	nopCleanup,	/* No need to clean up */
	rGeometry,	/* Device "geometry" */
	nopSignal,	/* System signals */
	rRandom,	/* Get randomness from kernel (alarm) */
	nopCancel,	
	nopSelect,
	NULL,
	NULL
};

static int rIoctl(Driver *dp, Message *msg) {
	Device *dv;
	if ((dv = rPrepare(msg->DEVICE)) == NIL_DEV)
	  return ENXIO;
	return doDrIoctl(&rDriverTable, msg);
}

static void rInit() {
	randomInit();
	rRandom(NULL, NULL);	/* Also set periodic timer */
}

int main(void) {
	rInit();	/* Initialize the random driver */
	driverTask(&rDriverTable);	/* Start driver's main loop */
	return OK;
}
