#include "../system.h"

int doDevIO(register Message *msg) {
	if (msg->DIO_REQUEST == DIO_INPUT) {
		switch (msg->DIO_TYPE) {
			case DIO_BYTE:
				msg->DIO_VALUE = inb(msg->DIO_PORT);
				break;
			case DIO_WORD:
				msg->DIO_VALUE = inw(msg->DIO_PORT);
				break;
			case DIO_LONG:
				msg->DIO_VALUE = inl(msg->DIO_PORT);
				break;
			default:
				return EINVAL;
		}
	} else {
		switch (msg->DIO_TYPE) {
			case DIO_BYTE:
				outb(msg->DIO_PORT, msg->DIO_VALUE);
				break;
			case DIO_WORD:
				outw(msg->DIO_PORT, msg->DIO_VALUE);
				break;
			case DIO_LONG:
				outl(msg->DIO_PORT, msg->DIO_VALUE);
				break;
		}
	}
	return OK;
}
