/* The drivers support the following operations (using message format m2):
 *
 *    m_type      DEVICE    PROC_NR     COUNT   POSITION   ADDRESS
 * ----------------------------------------------------------------
 * |  DEV_OPEN  | device  |	pNum	|		  |		    |		  |
 * |------------+---------+---------+---------+---------+---------|
 * |  DEV_CLOSE | device  |	pNum	|		  |		    |		  |
 * |------------+---------+---------+---------+---------+---------|
 * |  DEV_READ  | device  |	pNum	|  bytes  |	 offset | buf ptr |
 * |------------+---------+---------+---------+---------+---------|
 * |  DEV_WRITE | device  |	pNum	|  bytes  |  offset	| buf ptr |
 * |------------+---------+---------+---------+---------+---------|
 * | DEV_GATHER | device  |	pNum	| iov len |  offset	| buf ptr |
 * |------------+---------+---------+---------+---------+---------|
 * | DEV_SCATTER| device  |	pNum	| iov len |  offset	| buf ptr |
 * |------------+---------+---------+---------+---------+---------|
 * |  DEV_IOCTL | device  |	pNum	|func code|			| buf ptr | 
 * |------------+---------+---------+---------+---------+---------|
 * |  CANCEL	| device  |	pNum	| r/w	  |		    |		  |
 * |------------+---------+---------+---------+---------+---------|
 * |  HARD_STOP |         |			|		  |			|		  |
 * ----------------------------------------------------------------
 *
 *  The file contains one entry point:
 *
 *		driverTask:		called by the device dependent task entry
 */

#include "../drivers.h"
#include "sys/ioc_disk.h"
#include "driver.h"

#define dmaBytesLeft(addr)	((unsigned) 0x10000 - (unsigned) ((addr) & 0xFFFF))

static u8_t buffer[(unsigned) 2 * DMA_BUF_SIZE];
u8_t *tmpBuf;			/* The DMA buffer eventually */
phys_bytes tmpPhysAddr;		/* Phys address of DMA buffer */

static void initBuffer() {
/* Select a buffer that can safely be used for DMA transfers. It may also
 * be used to read partition tables and such. Its absolute address is
 * 'tmpPhysAddr', the normal address is 'tmpBuf'.
 */
	unsigned left;

	tmpBuf = buffer;
	sysUMap(SELF, D, (vir_bytes) buffer, (phys_bytes) sizeof(buffer), &tmpPhysAddr);

	if ((left = dmaBytesLeft(tmpPhysAddr)) < DMA_BUF_SIZE) {
		tmpBuf += left;
		tmpPhysAddr += left;
	}
}

int deviceCaller;

static int doRW(Driver *dp, Message *msg) {
/* Carry out a simple read or write request. */
	IOVec ioVec;
	int r, opCode;
	phys_bytes physAddr;

	if (msg->COUNT < 0)
	  return EINVAL;

	/* Check the user buffer. */
	sysUMap(msg->PROC_NR, D, (vir_bytes) msg->ADDRESS, msg->COUNT, &physAddr);
	if (physAddr == 0)
	  return EFAULT;

	/* Prepare for I/O. */
	if ((*dp->drPrepare)(msg->DEVICE) == NIL_DEV)
	  return ENXIO;

	/* Create a one element scatter/gather vector for the buffer. */
	opCode = msg->m_type == DEV_READ ? DEV_GATHER : DEV_SCATTER;
	ioVec.iov_addr = (vir_bytes) msg->ADDRESS;
	ioVec.iov_size = msg->COUNT;

	/* Transfer bytes from/to the device. */
	r = (*dp->drTransfer)(msg->PROC_NR, opCode, msg->POSITION, &ioVec, 1);

	/* Return the number of bytes transferred or an error code. */
	return r == OK ? (msg->COUNT - ioVec.iov_size) : r;
}

static int doVecRW(Driver *dp, Message *msg) {
/* Carry out an device read or write to/from a vector of user addresses.
 * The "user addresses" are assumed to be safe, i.e. FS transferring to/from
 * its own buffers, so they are not checked.
 */
	static IOVec ioVecs[NR_IO_REQS];
	IOVec *iov;
	phys_bytes size;
	unsigned numReq;
	int r;

	numReq = msg->COUNT;	/* Length of I/O vector */

	if (msg->m_source < 0) {
		/* Called by a task, no need to copy vector. */
		iov = (IOVec *) msg->ADDRESS;
	} else {
		/* Copy the vector from the caller to kernel space. */
		if (numReq > NR_IO_REQS) 
		  numReq = NR_IO_REQS;
		size = numReq * sizeof(ioVecs[0]);

		if (OK != sysDataCopy(msg->m_source, (vir_bytes) msg->ADDRESS,
						SELF, (vir_bytes) ioVecs, size))
		  panic((*dp->drName)(), "bad I/O vector by", msg->m_source);
		iov = ioVecs;
	}

	/* Prepare for I/O. */
	if ((*dp->drPrepare)(msg->DEVICE) == NIL_DEV)
	  return ENXIO;

	/* Transfer bytes from/to the device. */
	r = (*dp->drTransfer)(msg->PROC_NR, msg->m_type, msg->POSITION, iov, numReq);

	/* Copy the I/O vector back to the caller. */
	if (msg->m_source >= 0) 
	  sysDataCopy(SELF, (vir_bytes) ioVecs, 
			msg->m_source, (vir_bytes) msg->ADDRESS, size);

	return r;
}

void driverTask(Driver *dp) {
	int r, pNum;
	Message msg;

	/* Get a DMA buffer. */
	initBuffer();

	/* Here is the main loop of the disk task. It waits for a message, carries
	 * it out, and sends a reply.
	 */
	while (true) {
		/* Wait for a request to read or write a disk block. */
		if (receive(ANY, &msg) != OK)
		  continue;
		
		deviceCaller = msg.m_source;
		pNum = msg.PROC_NR;

		/* Now carry out the work. */
		switch (msg.m_type) {
			case DEV_OPEN:
				r = (*dp->drOpen)(dp, &msg);
				break;
			case DEV_CLOSE:
				r = (*dp->drClose)(dp, &msg);
				break;
			case DEV_IOCTL:
				r = (*dp->drIoctl)(dp, &msg);
				break;
			case CANCEL:
				r = (*dp->drCancel)(dp, &msg);
				break;
			case DEV_SELECT:
				r = (*dp->drSelect)(dp, &msg);
				break;

			case DEV_READ:
			case DEV_WRITE:
				r = doRW(dp, &msg);
				break;
			case DEV_GATHER:
			case DEV_SCATTER:
				r = doVecRW(dp, &msg);
				break;

			case HARD_INT:
				if (dp->drHwInt)
				  (*dp->drHwInt)(dp, &msg);
				continue;
			case SYS_SIG:
				(*dp->drSignal)(dp, &msg);
				continue;
			case SYN_ALARM:
				(*dp->drAlarm)(dp, &msg);
				continue;

			default:
				if (dp->drOther)
				  r = (*dp->drOther)(dp, &msg);
				else
				  r = EINVAL;
				break;
		}

		/* Clean up leftover state. */
		(*dp->drCleanup)();

		/* Finally , prepare and send the reply message. */
		if (r != EDONTREPLY) {
			msg.m_type = TASK_REPLY;
			msg.REP_PROC_NR = pNum;
			/* Status is # of bytes transferred or error code. */
			msg.REP_STATUS = r;
			send(deviceCaller, &msg);
		}
	}
}

char *noName() {
	static char name[] = "noName";
	return name;
}

int doNop(Driver *dp, Message *msg) {
/* Nothing there, or nothing to do. */

	switch (msg->m_type) {
		case DEV_OPEN:
			return ENODEV;
		case DEV_CLOSE:
			return OK;
		case DEV_IOCTL:
			return ENOTTY;
		default:
			return EIO;
	}
}

Device *nopPrepare(int device) {
/* Nothing to prepare for. */
	return NIL_DEV;
}

void nopCleanup() {
/* Nothing to clean up. */
}

void nopSignal(Driver *dp, Message *msg) {
/* Default action for signal is to ignore. */
}

void nopAlarm(Driver *dp, Message *msg) {
/* Ignore the leftover alarm. */
}

int nopCancel(Driver *dp, Message *msg) {
/* Nothing to do for cancel. */
	return OK;
}

int nopSelect(Driver *dp, Message *msg) {
/* Nothing to do for select. */
	return OK;
}

int doDrIoctl(Driver *dp, Message *msg) {
/* Carry out a partition setting/getting request. */
	Device *dv;
	Partition entry;
	int s;

	if (msg->REQUEST != DIOC_SET_PART && msg->REQUEST != DIOC_GET_PART) {
		if (dp->drOther) 
		  return dp->drOther(dp, msg);
		return ENOTTY;
	}

	/* Decode the message parameters. */
	if ((dv = (*dp->drPrepare)(msg->DEVICE)) == NIL_DEV)
	  return ENXIO;

	if (msg->REQUEST == DIOC_SET_PART) {
		/* Copy just this one partition table entry. */
		if ((s = sysDataCopy(msg->PROC_NR, (vir_bytes) msg->ADDRESS,
					SELF, (vir_bytes) &entry, sizeof(entry))) != OK)
		  return s;
		dv->dv_base = entry.base;
		dv->dv_size = entry.size;
	} else {
		/* Return a partition table entry and the geometry of the drive. */
		entry.base = dv->dv_base;
		entry.size = dv->dv_size;
		(*dp->drGeometry)(&entry);
		if ((s = sysDataCopy(SELF, (vir_bytes) &entry,
					msg->PROC_NR, (vir_bytes) msg->ADDRESS, sizeof(entry))) != OK)
		  return s;
	}
	return OK;
}

