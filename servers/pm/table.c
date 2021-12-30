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

	map(EXIT, doPmExit);
	//TODO map(FORK, doFork);
	map(WAIT, doWaitPid);
	map(WAITPID, doWaitPid);
	map(TIME, doTime);
	//TODO map(BRK, doBrk);
	map(GETPID, doGetSet);
	map(SETUID, doGetSet);
	map(GETUID, doGetSet);
	map(STIME, doSTime);
	//TODO map(PTRACE, doTrace);
	map(ALARM, doAlarm);
	map(PAUSE, doPause);
	map(KILL, doKill);
	map(TIMES, doTimes);
	map(SETGID, doGetSet);
	map(GETGID, doGetSet);
	//TODO map(EXEC, doExec);
	map(SETSID, doGetSet);
	map(GETPGRP, doGetSet);

	map(SIGACTION, doSigAction);
	map(SIGSUSPEND, doSigSuspend);
	map(SIGPENDING, doSigPending);
	map(SIGPROCMASK, doSigProcMask);
	map(SIGRETURN, doSigReturn);
	//TODO map(REBOOT, doReboot);
	//TODO map(SVRCTL, doSvrCtl);

	map(GETSYSINFO, doGetSysInfo);
	map(GETPROCNR, doGetProcNum);
	//TODO map(ALLOCMEM, doAllocMem);
	//TODO map(FREEMEM, doFreeMem);
	map(GETPRIORITY, doGetSetPriority);
	map(SETPRIORITY, doGetSetPriority);
	map(GETTIMEOFDAY, doTime);
}
