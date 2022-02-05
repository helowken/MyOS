#include "syslib.h"

static int sysVecIO(int type, int req, char *pair, int num) {
	Message msg;

	msg.DIO_TYPE = type;
	msg.DIO_REQUEST = req;
	msg.DIO_VEC_ADDR = pair;
	msg.DIO_VEC_SIZE = num;
	return taskCall(SYSTASK, SYS_VDEVIO, &msg);
}

int sysVecOutb(PvBytePair *pair, int num) {
	return sysVecIO(DIO_BYTE, DIO_OUTPUT, (char *) pair, num);
}

int sysVecOutw(PvWordPair *pair, int num) {
	return sysVecIO(DIO_WORD, DIO_OUTPUT, (char *) pair, num);
}

int sysVecOutl(PvLongPair *pair, int num) {
	return sysVecIO(DIO_LONG, DIO_OUTPUT, (char *) pair, num);
}

int sysVecInb(PvBytePair *pair, int num) {
	return sysVecIO(DIO_BYTE, DIO_INPUT, (char *) pair, num);
}

int sysVecInw(PvWordPair *pair, int num) {
	return sysVecIO(DIO_WORD, DIO_INPUT, (char *) pair, num);
}

int sysVecInl(PvLongPair *pair, int num) {
	return sysVecIO(DIO_LONG, DIO_INPUT, (char *) pair, num);
}

