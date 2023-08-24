#define _TABLE

#include "fs.h"
#include <minix/callnr.h>
#include <minix/com.h>


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
	map(LINK, doLink);		/* 9 = link */
	map(UNLINK, doUnlink);	/* 10 = unlink */
	map(CHDIR, doChdir);	/* 12 = chdir */
	map(MKNOD, doMknod);	/* 14 = mknod */
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
	map(SYNC, doSync);		/* 36 = sync */
	map(RENAME, doRename);	/* 38 = rename */
	map(MKDIR, doMkdir);	/* 39 = mkdir */
	map(RMDIR, doUnlink);	/* 40 = rmdir */
							/* 41 = dup TODO */
	map(PIPE, doPipe);		/* 42 = pipe */
	map(SETGID, doSet);		/* 46 = setgid */
	map(IOCTL, doIoctl);	/* 54 = ioctl */
	map(FCNTL, doFcntl);	/* 55 = fcntl */
	map(EXEC, doExec);		/* 59 = execve */
	map(UMASK, doUmask);	/* 60 = umask */
	map(CHROOT, doChroot);	/* 61 = chroot */
	map(SETSID, doSetSid);	/* 62 = setsid */
	map(UNPAUSE, doUnpause);	/* 65 = UNPAUSE */
							/* 67 = REVIVE TODO */
	map(REBOOT, doReboot);	/* 76 = reboot */
							/* 77 = svrctl TODO */
	map(GETSYSINFO, doGetSysInfo);	/* 79 = getsysinfo */
	map(DEVCTL, doDevCtl);	/* 81 = devctl */
	map(FSTATFS, doFStatfs);	/* 82 = fstatfs */
	map(SELECT, doSelect);	/* 85 = select */
	map(FCHDIR, doFchdir);	/* 86 = fchdir */
	map(FSYNC, doFsync);	/* 87 = fsync */
}



