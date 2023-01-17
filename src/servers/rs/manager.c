#include "rs.h"
#include "sys/wait.h"
#include "minix/dmap.h"

#define EXEC_FAILED		49		/* Arbitrary, recognizable status */
#define MAX_PATH_LEN	256		/* Maximum path string length */
#define MAX_ARGS_LEN	4096	/* Maximum argument string length */
#define MAX_ARG_COUNT	1		/* Parsed arguments count */

static char command[MAX_PATH_LEN + 1];
static char argBuf[MAX_ARGS_LEN + 1];

int doStart(Message *mp) {
	Message msg;
	int childProcNum;
	int majorNum;
	enum DevStyle devStyle;
	pid_t childPid;
	char *args[MAX_ARG_COUNT + 1];
	int s;

	/* Obtain command name and parameters. */
	if (mp->SRV_PATH_LEN > MAX_PATH_LEN)
	  return E2BIG;
	if ((s = sysDataCopy(mp->m_source, (vir_bytes) mp->SRV_PATH_ADDR,
				SELF, (vir_bytes) command, mp->SRV_PATH_LEN)) != OK)
	  return s;
	if (command[0] != '/')
	  return EINVAL;
	args[0] = command;

	if (mp->SRV_ARGS_LEN > 0) {
		if (mp->SRV_ARGS_LEN > MAX_ARGS_LEN)
		  return E2BIG;
		if ((s = sysDataCopy(mp->m_source, (vir_bytes) mp->SRV_ARGS_ADDR,
				SELF, (vir_bytes) argBuf, mp->SRV_ARGS_LEN)) != OK)
		  return s;
		argBuf[mp->SRV_ARGS_LEN] = '\0';
		args[1] = &argBuf[0];
		args[2] = NULL;
	} else {
		args[1] = NULL;
	}

	/* Now try to execute the new system service. Fork a new process. The child
	 * process will be inhibited from running by the NO_PRIV flag. Only let the
	 * child run once its privileges have been set by the parent.
	 */
	if ((s = taskCall(PM_PROC_NR, FORK, &msg)) < 0) 
	  report("SM", "taskCall to PM failed", s);
	childPid = msg.m_type;
	childProcNum = msg.PR_PROC_NR;

	/* Now branch for parent and child process, and check for error. */
	switch (childPid) {
		case 0:		/* Child process */
			execve(command, args, NULL);
			report("SM", "warning, exec() failed", errno);	/* Shouldn't happen */
			exit(EXEC_FAILED);		/* Terminate child */
			break;
		case -1:
			report("SM", "warning, fork() failed", errno);	/* Shouldn't happen */
			return errno;
		default:
			if ((majorNum = mp->SRV_DEV_MAJOR) > 0) {	/* Set driver map */
				devStyle = STYLE_DEV;
				if ((s = mapDriver(childProcNum, majorNum, devStyle)) < 0) 
				  report("SM", "couldn't map driver", errno);	
			}
			/* Set privileges to let child run. */
			if ((s = taskCall(SYSTEM, SYS_PRIVCTL, &msg)) < 0)	
			  report("SM", "taskCall to SYSTEM failed", s);
			printf("=== SM: started '%s %s', majorDev: %d, pid: %d, pNum: %d\n",
					command, argBuf, majorNum, childPid, childProcNum);
	}
	return OK;
}

int doStop(Message *msg) {
	return ENOSYS;
}

int doExit(Message *msg) {
	pid_t exitPid;
	int exitStatus;
	
	/* See which child exited and what the exit status is. This is done in a
	 * loop because multiple childs may have exited, all reported by one
	 * SIGCHLD signal. The WNOHANG options is used to prevent blocking if,
	 * somehow, no exited child can be found.
	 */
	while ((exitPid = waitpid(-1, &exitStatus, WNOHANG)) != 0) {
	}
	return OK;
}


