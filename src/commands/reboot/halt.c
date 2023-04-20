#define	_POSIX_SOURCE	1
#include "sys/types.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "string.h"
#include "errno.h"
#include "unistd.h"
#include "sys/stat.h"
#include "sys/wait.h"
#include "minix/minlib.h"

char *prog;

void writeLog(void);

static void usageErr() {
	fprintf(stderr, "Usage: %s [-hrRf] [-x reboot-code]\n", prog);
	exit(1);
}

int main(int argc, char **argv) {
	int flag = -1;	/* Default action unknown */
	int fast = 0;	/* Fast halt/reboot, don't bother being nice. */
	int i;
	struct stat dummy;
	char *monitorCode = "";
	pid_t pid;

	prog = getProg(argv);
	if (strcmp(prog, "halt") == 0)
	  flag = RBT_HALT;
	if (strcmp(prog, "reboot") == 0)
	  flag = RBT_REBOOT;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt = argv[i++] + 1;

		if (*opt == '-' && opt[1] == 0)	/* -- */
		  break;

		while (*opt != 0) {
			switch (*opt++) {
				case 'h':
					flag = RBT_HALT;
					break;
				case 'r':
					flag = RBT_REBOOT;
					break;
				case 'R':
					flag = RBT_RESET;
					break;
				case 'f':
					fast = 1;
					break;
				case 'x':
					flag = RBT_MONITOR;
					if (*opt == 0) {
						if (i == argc)
						  usageErr();
						opt = argv[i++];
					}
					monitorCode = opt;
					opt = "";
					break;
				default:
					usageErr();
			}
		}
	}

	if (i != argc)
	  usageErr();

	if (flag == -1) {
		fprintf(stderr, "Don't know what to do when named '%s'\n", prog);
		exit(1);
	}

	if (stat("/usr/bin", &dummy) < 0) {
		/* It seems that /usr isn't present, let's assume "-f." */
		fast = 1;
	}

	writeLog();
	if (fast) {
		/* But not too fast... */
		signal(SIGTERM, SIG_IGN);
		kill(1, SIGTERM);
		printf("Sending SIGTERM to all processes ...\n");
		kill(-1, SIGTERM);
		sleep(1);
	} else {
		/* Run the shutdown scripts. */
		signal(SIGHUP, SIG_IGN);
		signal(SIGTERM, SIG_IGN);

		switch ((pid = fork())) {
			case -1:
				fprintf(stderr, "%s: can't fork(): %s\n", prog, strerror(errno));
				exit(1);
			case 0:
				//TODO execl("/bin/sh", "sh", "/etc/rc", "down", (char *) NULL);
				fprintf(stderr, "%s: can't execute: /bin/sh: %s\n", prog, strerror(errno));
				exit(1);
			default:
				while (waitpid(pid, NULL, 0) != pid) {
				}
		}

		/* Tell init to stop spawning getty's. */
		kill(1, SIGTERM);

		/* Give everybody a chance to die peacefully. */
		printf("Sending SIGTERM to all processes ...\n");
		kill(-1, SIGTERM);
		sleep(2);
	}

	reboot(flag, monitorCode, strlen(monitorCode));
	fprintf(stderr, "%s: reboot(): %s\n", prog, strerror(errno));
	return 1;
}



