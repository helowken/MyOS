#include <stdio.h>
#include <stdlib.h>

#define NEXITS	32

void (*__funcTable[NEXITS])();
int __funcCount = 0;

extern void _exit(int);

/* Only flush output buffers when necessary */
int (*_clean)() = NULL;

static void _calls() {
	register int i = __funcCount;

	/* Called in reversed order of their registration */
	while (--i >= 0) {
		(*__funcTable[i])();
	}
}

void exit(int status) {
	_calls();
	if (_clean)
	  _clean();
	_exit(status);
}
