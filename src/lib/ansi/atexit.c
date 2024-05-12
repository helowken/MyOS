#include <stdlib.h>

#define N_EXITS	32

extern void (*__funcTable[N_EXITS])(void);
extern int __funcCount;

int atexit(void (*func)(void)) {
	if (__funcCount >= N_EXITS)
	  return 1;
	__funcTable[__funcCount++] = func;
	return 0;
}
