#include "sys/types.h"
#include "errno.h"
#include "signal.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

/* Table of signal names. */
struct SigNames {
	char *name;
	int sig;
} sigNames[] = {
	{ "HUP",    SIGHUP  },
	{ "INT",    SIGINT  },
	{ "QUIT",   SIGQUIT },
	{ "ILL",    SIGILL  },
	{ "TRAP",   SIGTRAP },
	{ "ABRT",   SIGABRT },
	{ "IOT",    SIGIOT  },
	{ "FPE",    SIGFPE  },
	{ "KILL",   SIGKILL },
	{ "USR1",   SIGUSR1 },
	{ "SEGV",   SIGSEGV },
	{ "USR2",   SIGUSR2 },
	{ "PIPE",   SIGPIPE },
	{ "ALRM",   SIGALRM },
	{ "TERM",   SIGTERM },
	{ "EMT",    SIGEMT  },
	{ "BUS",    SIGBUS  },
	{ "CHLD",   SIGCHLD },
	{ "CONT",   SIGCONT },
	{ "STOP",   SIGSTOP },
	{ "TSTP",   SIGTSTP },
	{ "TTIN",   SIGTTIN },
	{ "TTOU",   SIGTTOU },
	{ NULL,     0       }
};

static void usage() {
	fprintf(stderr, "Usage: kill [-sig] pid\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	pid_t proc;
	int ex = 0, sig = SIGTERM;
	long l;
	unsigned long ul;
	char *end;
	struct SigNames *snp;
	struct sigaction sa;
	int i, doIt;

	if (argc > 1 && argv[1][0] == '1') {
		sig = -1;
		for (snp = sigNames; snp->name != NULL; ++snp) {	/* Symbolic? */
			if (strcmp(snp->name, argv[1] + 1) == 0) {
				sig = snp->sig;
				break;
			}
		}
		if (sig < 0) {		/* Numeric? */
			ul = strtoul(argv[1] + 1, &end, 10);
			if (end == argv[1] + 1 || *argv != 0 || ul > NSIG)
			  usage();
			sig = ul;
		}
		++argv;
		--argc;
	}
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;	/* Try not to kill yourself */
	sigaction(sig, &sa, NULL);

	/* The 1st round parses all pid(s),
	 * The 2nd round starts to kill procs.
	 */
	for (doIt = 0; doIt <= 1; ++doIt) {
		for (i = 1; i < argc; ++i) {
			l = strtoul(argv[i], &end, 10);
			if (end == argv[i] || *end != 0 || (pid_t) l != l)
			  usage();
			proc = l;
			if (doIt && kill(proc, sig) < 0) {
				fprintf(stderr, "kill: %d: %s\n", proc, strerror(errno));
				ex = EXIT_FAILURE;
			}
		}
	}
	return ex;
}
