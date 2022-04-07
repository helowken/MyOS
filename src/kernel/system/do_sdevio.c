#include "../system.h"
#include "minix/devio.h"

int doStrDevIO(register Message *msg) {
	int pNum = msg->DIO_VEC_PROC;
	int count = msg->DIO_VEC_SIZE;
	long port = msg->DIO_PORT;
	phys_bytes physBuf;

	/* Check if process number is OK. A process number is allowed here, because
	 * driver may directly provide a pointer to a buffer at the user-process
	 * that initiated the device I/O. Kernel processes, of course, are denied.
	 */
	if (pNum == SELF)
	  pNum = msg->m_source;
	if (! isOkProcNum(pNum))
	  return EINVAL;
	if (isKernelNum(pNum))
	  return EPERM;

	/* Get and check physical address. */
	if ((physBuf = nUMapLocal(pNum, (vir_bytes) msg->DIO_VEC_ADDR, count)) == 0)
	  return EFAULT;

	/* Perform device I/O for bytes and words. Longs are not supported. */
	if (msg->DIO_REQUEST == DIO_INPUT) {
		switch (msg->DIO_TYPE) {
			case DIO_BYTE:
				physInsb(port, physBuf, count);
				break;
			case DIO_WORD:
				physInsw(port, physBuf, count);
				break;
			default:
				return EINVAL;
		}
	} else if (msg->DIO_REQUEST == DIO_OUTPUT) {
		switch (msg->DIO_TYPE) {
			case DIO_BYTE:
				physOutsb(port, physBuf, count);
				break;
			case DIO_WORD:
				physOutsw(port, physBuf, count);
				break;
			default:
				return EINVAL;
		}
	} else {
		return EINVAL;
	}
	return OK;
}
