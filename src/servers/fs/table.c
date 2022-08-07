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

	map(OPEN, doOpen);		/* 5 = open */
	map(FSTAT, doFstat);	/* 28 = fstat */
	map(MKDIR, doMkdir);	/* 39 = mkdir */
	map(FCNTL, doFcntl);	/* 55 = fcntl */
}



