#define _TABLE

#include "pm.h"
#include "minix/callnr.h"
#include "signal.h"
#include "mproc.h"

#define map(callNum, handler)	callVec[callNum] = (handler)

int (*callVec[NCALLS])();

void initSysCalls() {
	int i;

	for (i = 0; i < NCALLS; ++i) {
		callVec[i] = noSys;
	}

	map(EXIT, doPmExit);		/* 1 = exit */
	map(FORK, doFork);			/* 2 = fork */
	map(WAIT, doWaitPid);		/* 7 = wait */
	map(WAITPID, doWaitPid);	/* 11 = waitpid */
	map(TIME, doTime);			/* 13 = time */
	map(BRK, doBrk);			/* 17 = break */
	map(GETPID, doGetSet);		/* 20 = getpid */
	map(SETUID, doGetSet);		/* 23 = setuid */
	map(GETUID, doGetSet);		/* 24 = getuid */
	map(STIME, doSTime);		/* 25 = stime */
	//TODO map(PTRACE, doTrace);	/* 26 = ptrace */
	map(ALARM, doAlarm);		/* 27 = alarm */
	map(PAUSE, doPause);		/* 29 = pause */
	map(KILL, doKill);			/* 37 = kill */
	map(TIMES, doTimes);		/* 43 = times */
	map(SETGID, doGetSet);		/* 46 = setgid */
	map(GETGID, doGetSet);		/* 47 = getgid */
	map(EXEC, doExec);			/* 59 = execve */
	map(SETSID, doGetSet);		/* 62 = setsid */
	map(GETPGRP, doGetSet);		/* 63 = getpgrp */

	map(SIGACTION, doSigAction);	/* 71 = sigaction */
	map(SIGSUSPEND, doSigSuspend);	/* 72 = sigsuspend */
	map(SIGPENDING, doSigPending);	/* 73 = sigpending */
	map(SIGPROCMASK, doSigProcMask);	/* 74 = sigprocmask */
	map(SIGRETURN, doSigReturn);	/* 75 = sigreturn */
	//TODO map(REBOOT, doReboot);	/* 76 = reboot */
	map(SVRCTL, doSvrCtl);	/* 77 = svrctl */

	map(GETSYSINFO, doGetSysInfo);	/* 79 = getsysinfo */
	map(GETPROCNR, doGetProcNum);	/* 80 = getprocnr */
	map(ALLOCMEM, doAllocMem);		/* 83 = memalloc */
	//TODO map(FREEMEM, doFreeMem);	/* 84 = memfree */
	map(GETPRIORITY, doGetSetPriority);	/* 88 = getpriority */
	map(SETPRIORITY, doGetSetPriority);	/* 89 = setpriority */
	map(GETTIMEOFDAY, doTime);		/* 90 = gettimeofday */
}
