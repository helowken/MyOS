#include "../system.h"
#include "minix/devio.h"

/* Buffer for SYS_VDEVIO to copy (port, value)-pair from/to user. */
static char vDevIOBuf[VDEVIO_BUF_SIZE];
static PvBytePair *pvb = (PvBytePair *) vDevIOBuf;
static PvWordPair *pvw = (PvWordPair *) vDevIOBuf;
static PvLongPair *pvl = (PvLongPair *) vDevIOBuf;

int doVecDevIO(Message *msg) {
/* Perform a series of device I/O on behalf of a non-kernel process. The
 * I/O addresses and I/O values are fetched from and returned to some buffer
 * in user space. The actual I/O is wrapped by lock() and unlock() to present
 * that I/O batch from being interrupted..
 * This is the counterpart of doDevIO, which performs a single device I/O.
 */
	int vecSize;
	bool ioInput;
	size_t bytes;
	int caller;
	vir_bytes callerVirAddr;
	phys_bytes callerPhysAddr;
	int i;

	/* Get the request, size of the request vector, and check the values. */
	if (msg->DIO_REQUEST == DIO_INPUT)
	  ioInput = true;
	else if (msg->DIO_REQUEST == DIO_OUTPUT)
	  ioInput = false;
	else
	  return EINVAL;

	if ((vecSize = msg->DIO_VEC_SIZE) <= 0)
	  return EINVAL;

	switch (msg->DIO_TYPE) {
		case DIO_BYTE:
			bytes = vecSize * sizeof(PvBytePair);
			break;
		case DIO_WORD:
			bytes = vecSize * sizeof(PvWordPair);
			break;
		case DIO_LONG:
			bytes = vecSize * sizeof(PvLongPair);
			break;
		default:
			return EINVAL;
	}
	if (bytes > sizeof(vDevIOBuf))
	  return E2BIG;

	/* Calculate physical addresses and copy (port, value)-pairs from user. */
	caller = msg->m_source;
	callerVirAddr = (vir_bytes) msg->DIO_VEC_ADDR;
	callerPhysAddr = umapLocal(procAddr(caller), D, callerVirAddr, bytes);
	if (callerPhysAddr == 0)
	  return EFAULT;
	physCopy(callerPhysAddr, vir2Phys(vDevIOBuf), (phys_bytes) bytes);

	/* Perform actual device I/O for byte, word, and long values. Note that
	 * the entire switch is wrapped in lock() and unlock() to prevent the I/O
	 * batch from being interrupted.
	 */
	lock(13, "doVecDevIO");
	switch (msg->DIO_TYPE) {
		case DIO_BYTE:
			if (ioInput) {
				for (i = 0; i < vecSize; ++i) {
					pvb[i].value = inb(pvb[i].port);					
				}
			} else {
				for (i = 0; i < vecSize; ++i) {
					outb(pvb[i].port, pvb[i].value);
				}
			}
			break;
		case DIO_WORD:
			if (ioInput) {
				for (i = 0; i < vecSize; ++i) {
					pvw[i].value = inw(pvw[i].port);					
				}
			} else {
				for (i = 0; i < vecSize; ++i) {
					outw(pvw[i].port, pvw[i].value);
				}
			}
			break;
		case DIO_LONG:
			if (ioInput) {
				for (i = 0; i < vecSize; ++i) {
					pvl[i].value = inl(pvl[i].port);					
				}
			} else {
				for (i = 0; i < vecSize; ++i) {
					outl(pvl[i].port, pvl[i].value);
				}
			}
			break;
		default:
			return EINVAL;
	}
	unlock(13);

	/* Almost done, copy back results for input requests. */
	if (ioInput) 
	  physCopy(vir2Phys(vDevIOBuf), callerPhysAddr, (phys_bytes) bytes);

	return OK;
}
