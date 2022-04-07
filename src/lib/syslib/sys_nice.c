#include "syslib.h"

int sysNice(int pNum, int priority) {
	Message msg;

	msg.m1_i1 = pNum;
	msg.m1_i2 = priority;
	return taskCall(SYSTASK, SYS_NICE, &msg);
}
