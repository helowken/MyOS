#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE	1
#endif

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <minix/minlib.h>

#if __minix
static char DEV_LOG[] = "/dev/log";
#else
static char DEV_LOG[] = "/dev/console";
#endif
static char *prog;

static void usageErr() {
	usage(prog, "[-d] -[t seconds] command [arg ...]");
}

int main(int argc, char **argv) {
	int fd;
	unsigned n = 0;
	int daemonize = 0;
	int i;

	prog = getProg(argv);
	i = 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt = argv[i++] + 1;
		char *end;
		unsigned long sec;

		if (opt[0] == '-' && opt[1] == 0)
		  break;
	
		while (*opt != 0) {
			switch (*opt++) {
				case 'd':	/* -d */
					daemonize = 1;
					break;
				case 't':	/* -t n: alarm in n seconds */
					if (*opt == 0) {
						if (i == argc)
						  usageErr();
						opt = argv[i++];
					}
					sec = strtoul(opt, &end, 10);
					if (end == opt || *end != 0 || (n = sec) != sec)
					  usageErr();
					opt = "";
					break;
				default:
					usageErr();

			}
		}
	}

	if ((argc - 1) < 1)
	  usageErr();

	/* Try to open the controlling tty. */
	if ((fd = open("/dev/tty", O_RDWR)) < 0) {
		if (errno != ENXIO)
		  fatal(prog, "/dev/tty");
	}

	if (!daemonize) {

		/* Bring to the foreground. If we already have a controlling
		 * tty then use it. Otherwise try to allocate the console as
		 * controlling tty and begin a process group.
		 */
		if (fd < 0) {
			if (setsid() < 0)
			  fatal(prog, "setsid()");
			fd = open("/dev/console", O_RDWR);

		}

		if (fd >= 0) {
			if (fd != STDIN_FILENO) {
				dup2(fd, STDIN_FILENO);
				close(fd);
			}
			dup2(STDIN_FILENO, STDOUT_FILENO);
			dup2(STDIN_FILENO, STDERR_FILENO);
		}

		/* Set the usual signals back to the default. */
		signal(SIGHUP, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
	} else {
		/* Send to the background. Redirect input to /dev/null, and
		 * output to the log device. Detach from the process group.
		 */
		if (fd >= 0) {
			close(fd);
			if (setsid() < 0)
			  fatal(prog, "setsid()");
		}
		if ((fd = open("/dev/null", O_RDWR)) < 0)
		  fatal(prog, "/dev/null");
		if (fd != STDIN_FILENO) {
			dup2(fd, STDIN_FILENO);
			close(fd);
		}
		if ((fd = open(DEV_LOG, O_WRONLY)) < 0)
		  fatal(prog, DEV_LOG);
		if (fd != STDOUT_FILENO) {
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		dup2(STDOUT_FILENO, STDERR_FILENO);

		/* Move to the root directory. */
		chdir("/");
	}

	/* Schedule the alarm. (It is inherited over execve.) */
	if (n != 0)
	  alarm(n);

	/* Call program. */
	execvp(argv[i], argv + i);

	/* Complain. */
	fatal(prog, argv[i]);
	return 0;
}


