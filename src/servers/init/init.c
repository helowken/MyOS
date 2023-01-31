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
#include "minix/minlib.h"

#include "minix/ipc.h" //TODO delete
#include "stdio.h"	//TODO delete
#include "string.h" //TODO delete

static int gotHup = 0;		/* Flag, showing signal 1 was received */
static int gotAbrt = 1;		/* Flag, showing signal 6 was received */
static int spawn = 1;		/* Flag, spawn processes only when set */

static int execute(char **cmd) {
/* Execute a command with a path search along /sbin:/bin:/usr/sbin:/usr/bin. */
	static char *nullEnv[] = { NULL };
	char command[128];
	char *path[] = { "/bin"};//TODO, "/sbin", "/usr/sbin", "/usr/bin" };
	int i;

	if (cmd[0][0] == '/') {
		/* A full path. */
		return execve(cmd[0], cmd, nullEnv);
	}

	/* Path search. */
	for (i = 0; i < 1; ++i) {	//TODO
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
	printf("==== init errno: %d\n", errno);
	return -1;
}

static void onHup(int sig) {
	gotHup = 1;
	spawn = 1;
}

static void onTerm(int sig) {
	spawn = 0;
}

static void onAbrt(int sig) {
	static int count;

	if (++count == 2)
	  reboot(RBT_HALT);
	gotAbrt = 1;
}

void main() {
	pid_t pid;		/* Pid of child process */
	struct stat st;
	struct sigaction sa;
	Message msg;

	if (fstat(0, &st) < 0) {
		/* Open standard input, output & error. */
		open("/dev/null", O_RDONLY);
		open("/dev/log", O_WRONLY);
		dup(1);		/* 2 > &1 */
	}

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	/* Hangup: Reexamine /etc/ttytab for newly enabled terminal lines. */
	sa.sa_handler = onHup;
	sigaction(SIGHUP, &sa, NULL);

	/* Terminate: Stop spawning login processes, shutdown is near. */
	sa.sa_handler = onTerm;
	sigaction(SIGTERM, &sa, NULL);

	/* Abort: Sent by the kernel on CTRL-ALT-DEL; shut the system down. */
	sa.sa_handler = onAbrt;
	sigaction(SIGABRT, &sa, NULL);

	printf("====== init start =====\n"); //TODO
	/* Execute the /etc/rc file. */
	if ((pid = fork()) != 0) {
		/* Parent just waits. TODO*/
		while (wait(NULL) != pid) {
			if (gotAbrt)
			  reboot(RBT_HALT);
		}
	} else {
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
		report("sh /etc/rc", NULL);
		_exit(1);	/* Impossible, we hope */
	}

	//TODO
	
	// TODO start
	printf("====== init end =====\n");
	receive(0x7ace, &msg);
	//TODO end
}
