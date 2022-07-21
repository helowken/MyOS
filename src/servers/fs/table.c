#define _TABLE

#include "fs.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "lock.h"










int (*callVec[])() = {
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	doFstat,	/* 28 = fstat */
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,

	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,

	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys,
	noSys
};
