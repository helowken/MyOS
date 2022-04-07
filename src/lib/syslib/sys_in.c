#include "syslib.h"

int sysIn(int port, unsigned long *value, int type) {
	Message msg;
	int result;

	msg.DIO_TYPE = type;
	msg.DIO_REQUEST = DIO_INPUT;
	msg.DIO_PORT = port;

	result = taskCall(SYSTASK, SYS_DEVIO, &msg);
	*value = msg.DIO_VALUE;
	return result;
}
