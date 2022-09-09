#define _TABLE

#include "fs.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "lock.h"


#define map(callNum, handler)	callVec[callNum] = (handler)

int (*callVec[NCALLS])();

void initSysCalls() {
	int i;

	for (i = 0; i < NCALLS; ++i) {
		callVec[i] = noSys;
	}

	map(FORK, doFork);		/* 2 = fork */
	map(READ, doRead);		/* 3 = read */
	map(OPEN, doOpen);		/* 5 = open */
	map(CLOSE, doClose);	/* 6 = close */
	map(CHDIR, doChdir);	/* 12 = chdir */
	map(SETUID, doSet);		/* 23 = setuid */
	map(FSTAT, doFstat);	/* 28 = fstat */
	map(ACCESS, doAccess);	/* 33 = access */
	map(MKDIR, doMkdir);	/* 39 = mkdir */
	map(FCNTL, doFcntl);	/* 55 = fcntl */
}



