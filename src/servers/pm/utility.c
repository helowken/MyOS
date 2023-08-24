#include "pm.h"
#include <sys/stat.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <fcntl.h>
#include <signal.h>
#include "mproc.h"
#include "param.h"

#include <string.h>
#include "../../kernel/const.h"
#include "../../kernel/config.h"
#include "../../kernel/type.h"
#include "../../kernel/proc.h"

void panic(char *who, char *msg, int num) {
/* An unrecoverable error has occurred. Panics are caused when an internal
 * inconsistency is detected, e.g., a programming error or illegal value of a
 * defined constant. The process manager decides to shut down. This results
 * in a HARD_STOP notification to all system processes to allow local cleanup.
 */
	printf("PM panic (%s): %s", who, msg);
	if (num != NO_NUM)
	  printf(": %d", num);
	printf("\n");
	sysAbort(RBT_PANIC);
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
	int r;

	msg.m_tell_fs_arg1 = p1;
	msg.m_tell_fs_arg2 = p2;
	msg.m_tell_fs_arg3 = p3;
	if ((r = taskCall(FS_PROC_NR, what, &msg)) != OK) {
		printf("=== pm call fs failed: %d\n", what);
	  panic(__FILE__, "tell fs failed", r);
	}
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

int checkAllowed(char *pathName, struct stat *st, int mask) {
/* Check to see if file can be accessed. Return ECCES or ENOENT if the access
 * is prohibited. If it is legal open the file and return a file descriptor.
 */
	int fd;
	int savedErrno;

	/* TellFS(DO_CHDIR, ...) has set PM's real ids to the user's effective 
	 * ids, so access() works right for setuid programs.
	 */
	if (access(pathName, mask) < 0)  
	  return -errno;

	/* The file is accessible but might not be readable. Make it readable. */
	tellFS(SETUID, PM_PROC_NR, (int) SUPER_USER, (int) SUPER_USER);

	/* Open the file and fstat it. Restore the ids early to handle errors. */
	fd = open(pathName, O_RDONLY | O_NONBLOCK);
	savedErrno = errno;		/* Open might fail, e.g. from ENFILE */
	tellFS(SETUID, PM_PROC_NR, (int) currMp->mp_euid, (int) currMp->mp_euid);
	if (fd < 0) 
	  return -savedErrno;
	if (fstat(fd, st) < 0)
	  panic(__FILE__, "checkAllowed: fstat failed", NO_NUM);

	/* Only regular files can be executed. 
	 * Use the fact that mask for access() is the same as the permissions mask.
	 * E.g., X_BIT in <minix/const.h> is the same as X_OK in <unistd.h> and
	 * S_IXOTH in <sys/stat.h>.
	 */ 
	if (mask == X_BIT && (st->st_mode & I_TYPE) != I_REGULAR) {
		close(fd);
		return EACCES;
	}
	return fd;
}






