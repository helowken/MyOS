#include "syslib.h"

int sysStrDevIO(int req, long port, int type, int pNum, void *buffer, int count) {
	Message msg;

	msg.DIO_REQUEST = req;
	msg.DIO_TYPE = type;
	msg.DIO_PORT = port;
	msg.DIO_VEC_PROC = pNum;
	msg.DIO_VEC_ADDR = buffer;
	msg.DIO_VEC_SIZE = count;

	return taskCall(SYSTASK, SYS_SDEVIO, &msg);
}
