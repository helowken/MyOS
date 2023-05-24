#include <lib.h>
#include <unistd.h>

extern char *_brkSize;

int brk(char *addr) {
	Message msg;

	if (addr != _brkSize) {
		msg.m1_p1 = addr;
		if (syscall(MM, BRK, &msg) < 0)
		  return -1;
		_brkSize = msg.m2_p1;
	}
	return 0;
}

char *sbrk(int incr) {
	char *newSize, *oldSize;

	oldSize = _brkSize;
	newSize = _brkSize + incr;
	if ((incr > 0 && newSize < oldSize) || (incr < 0 && newSize > oldSize))
	  return (char *) -1;
	if (brk(newSize) == 0)
	  return oldSize;
	return (char *) -1;
}
