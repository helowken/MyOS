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

	map(EXIT, doExit);		/* 1 = exit */
	map(FORK, doFork);		/* 2 = fork */
	map(READ, doRead);		/* 3 = read */
	map(WRITE, doWrite);	/* 4 = write */
	map(OPEN, doOpen);		/* 5 = open */
	map(CLOSE, doClose);	/* 6 = close */
	map(CREAT, doCreat);	/* 8 = creat */
	map(UNLINK, doUnlink);	/* 10 = unlink */
	map(CHDIR, doChdir);	/* 12 = chdir */
	map(CHMOD, doChmod);	/* 15 = chmod */
	map(CHOWN, doChown);	/* 16 = chown */
	map(STAT, doStat);		/* 18 = stat */
	map(LSEEK, doLseek);	/* 19 = lseek */
	map(MOUNT, doMount);	/* 21 = mount */
	map(UMOUNT, doUmount);	/* 22 = umount */
	map(SETUID, doSet);		/* 23 = setuid */
	map(STIME, doSTime);	/* 25 = stime */
	map(FSTAT, doFstat);	/* 28 = fstat */
	map(UTIME, doUTime);	/* 30 = utime */
	map(ACCESS, doAccess);	/* 33 = access */
	map(RENAME, doRename);	/* 38 = rename */
	map(MKDIR, doMkdir);	/* 39 = mkdir */
	map(RMDIR, doUnlink);	/* 40 = rmdir */
	map(PIPE, doPipe);		/* 42 = pipe */
	map(SETGID, doSet);		/* 46 = setgid */
	map(IOCTL, doIoctl);	/* 54 = ioctl */
	map(FCNTL, doFcntl);	/* 55 = fcntl */
	map(EXEC, doExec);		/* 59 = execve */
	map(UMASK, doUmask);	/* 60 = umask */
	map(SETSID, doSetSid);	/* 62 = setsid */
	map(DEVCTL, doDevCtl);	/* 81 = devctl */
	map(FCHDIR, doFChdir);	/* 86 = fchdir */
}



