#include "pm.h"
#include "mproc.h"
#include "param.h"

#include "string.h"
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"
#include "../../kernel/proc.h"

void panic(char *who, char *msg, int num) {
	// TODO
}

char *findParam(const char *name) {
	register const char *namep;
	register char *envp;

	for (envp = (char *) monitorParams; *envp != 0;) {
		for (namep = name; *namep != 0 && *namep == *envp; ++namep, ++envp) {
		}
		if (*namep == '\0' && *envp == '=')
		  return envp + 1;
		while (*envp++ != 0) {
		}
	}
	return NULL;
}

int getMemMap(int pNum, MemMap *memMap) {
	Proc p;
	int s;

	if ((s = sysGetProc(&p, pNum)) != OK)
	  return s;
	memcpy(memMap, p.p_memmap, sizeof(p.p_memmap));
	return OK;
}

pid_t getFreePid() {
	static pid_t nextPid = INIT_PID;	/* Next pid to be assigned */
	register MProc *rmp;
	int t;

	/* Find a free pid for the child and put it in the table. */
	do {
		t = 0;
		nextPid = nextPid < NR_PIDS ? nextPid + 1 : INIT_PID + 1;
		for (rmp = &mprocTable[0]; rmp < &mprocTable[NR_PROCS]; ++rmp) {
			if (rmp->mp_pid == nextPid || rmp->mp_proc_grp == nextPid) {
				t = 1;
				break;
			}
		}
	} while (t);
	return nextPid;
}

int noSys() {
	return ENOSYS;
}

void tellFS(int what, int p1, int p2, int p3) {
/* This routine is only used by PM to inform FS of certain events:
 *		tellFS(CHDIR, slot, dir, 0)
 *		tellFS(EXEC, proc, 0, 0)
 *		tellFS(EXIT, proc, 0, 0)
 *		tellFS(FORK, parent, child, pid)
 *		tellFS(SETGID, proc, rgid, egid)
 *		tellFS(SETSID, proc, 0, 0)
 *		tellFS(SETUID, proc, ruid, euid)
 *		tellFS(UNPAUSE, proc, sigNum, 0)
 *		tellFS(STIME, time, 0, 0)
 */
	Message msg;

	msg.tell_fs_arg1 = p1;
	msg.tell_fs_arg2 = p2;
	msg.tell_fs_arg3 = p3;
	taskCall(FS_PROC_NR, what, &msg);
}

int getStackPtr(int pNum, vir_bytes *sp) {
	Proc p;
	int s;

	if ((s = sysGetProc(&p, pNum)) != OK)
	  return s;
	*sp = p.p_reg.esp;
	return OK;
}

int getPNumFromPid(pid_t pid) {
	int pNum;

	for (pNum = 0; pNum < NR_PROCS; ++pNum) {
		if (mprocTable[pNum].mp_pid == pid)
		  return pNum;
	}
	return -1;
}
