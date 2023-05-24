#include <lib.h>
#include <unistd.h>
#include <string.h>

int findProc(char *procName, int *pNum) {
	Message msg;

	msg.m1_p1 = procName;
	msg.m1_i1 = -1;
	msg.m1_i2 = strlen(procName);
	if (syscall(MM, GETPROCNR, &msg) < 0)
	  return -1;
	*pNum = msg.m1_i1;
	return 0;
}
