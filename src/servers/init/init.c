#include "minix/type.h"
#include "sys/types.h"
#include "sys/wait.h"
#include "sys/stat.h"
#include "sys/svrctl.h"
#include "ttyent.h"
#include "errno.h"
#include "fcntl.h"
#include "limits.h"
#include "signal.h"
#include "string.h"
#include "time.h"
#include "stdlib.h"
#include "unistd.h"
#include "utmp.h"

#include "minix/ipc.h" //TODO
#include "stdio.h"	//TODO
#include "string.h" //TODO

static int execute(char **cmd) {
/* Execute a command with a path search along /sbin:/bin:/usr/sbin:/usr/bin. */
	static char *nullEnv[] = { NULL };
	char command[128];
	char *path[] = { "/sbin", "/bin", "/usr/sbin", "/usr/bin" };
	int i;

	if (cmd[0][0] == '/') {
		/* A full path. */
		return execve(cmd[0], cmd, nullEnv);
	}

	/* Path search. */
	for (i = 0; i < 4; ++i) {
		if (strlen(path[i]) + 1 + strlen(cmd[0]) + 1 > sizeof(command)) {
			errno = ENAMETOOLONG;
			return -1;
		}
		strcpy(command, path[i]);
		strcat(command, "/");
		strcat(command, cmd[0]);
		execve(command, cmd, nullEnv);
		if (errno != ENOENT)
		  break;
	}
	printf("==== errno: %d\n", errno);
	return -1;
}

void main() {
	pid_t pid;		/* Pid of child process */
	struct stat st;
	struct sigaction sa;

	if (fstat(0, &st) < 0) {
		/* Open standard input, output & error. */
		open("/dev/null", O_RDONLY);
		open("/dev/log", O_WRONLY);
		dup(1);		/* 2 > &1 */
	}

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	/* Hangup: Reexamine /etc/ttytab for newly enabled terminal lines. */
	//TODO sa.sa_handler = onHup;
	sigaction(SIGHUP, &sa, NULL);

	/* Terminate: Stop spawning login processes, shutdown is near. */
	//TODO sa.sa_handler = onTerm;
	sigaction(SIGTERM, &sa, NULL);

	/* Abort: Sent by the kernel on CTRL-ALT-DEL; shut the system down. */
	//TODO sa.sa_handler = onAbort;
	sigaction(SIGABRT, &sa, NULL);

	printf("====== init start =====\n"); //TODO

	if ((pid = fork()) != 0) {
		/* Parent just waits. TODO*/
		printf("========== I'm parent\n");
	} else {
		printf("========== I'm child\n");
		SysGetEnv sysGetEnv;
		char bootOpts[16];
		static char *rcCommand[] = { "sh", "/etc/rc", NULL, NULL, NULL }; 
		char **rcp = rcCommand + 2;
	
		/* Get the boot options from the boot environment. */
		sysGetEnv.key = "bootopts";
		sysGetEnv.keyLen = 8 + 1;
		sysGetEnv.val = bootOpts;
		sysGetEnv.valLen = sizeof(bootOpts);
		if (svrctl(MM_GET_PARAM, &sysGetEnv) == 0)
		  *rcp++ = bootOpts;
		*rcp = "start";

		execute(rcCommand);
		//TODO

	}
	
	// TODO start
	printf("====== init end =====\n");
	Message msg;
	receive(0x7ace, &msg);
	//TODO end
}
