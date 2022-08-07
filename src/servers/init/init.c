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

static char x[20];//TODO

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
		/* Parent just waits. */
		strcpy(x, "abc");
		printf("========== I'm parent: %s\n", x);
	} else {
		strcpy(x, "def");
		printf("========== I'm child: %s\n", x);
	}
	
	// TODO start
	printf("====== init end =====\n");
	Message msg;
	receive(0x7ace, &msg);
	//TODO end
}
