#include "syslib.h"

int sysOut(int port, unsigned long value, int type) {
	Message msg;

	msg.DIO_TYPE = type;
	msg.DIO_REQUEST = DIO_OUTPUT;
	msg.DIO_PORT = port;
	msg.DIO_VALUE = value;

	return taskCall(SYSTASK, SYS_DEVIO, &msg);
}
