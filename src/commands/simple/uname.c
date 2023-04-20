#include "sys/types.h"
#include "sys/utsname.h"
#include "stdarg.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "minix/minlib.h"

/* Define the uname components. */
#define ALL			((unsigned) 0x1F)
#define	SYS_NAME	((unsigned) 0x01)
#define NODE_NAME	((unsigned) 0x02)
#define RELEASE		((unsigned) 0x04)
#define VERSION		((unsigned) 0x08)
#define U_MACHINE	((unsigned) 0x10)
#define ARCH		((unsigned) 0x20)

static char *prog;

static void print(int fd, ...) {
	va_list ap;
	char *p;

	va_start(ap, fd);
	while (1) {
		p = va_arg(ap, char *);
		if (p == (char *) NULL)
		  break;
		write(fd, p, strlen(p));
	}
	va_end(ap);
}

static void usageErr() {
	print(STDERR_FILENO, "Usage: ", prog, " -snrvmpa\n", (char *) NULL);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	int info;
	char *p;
	struct utsname un;
	int i = 0;

	prog = getProg(argv);

	for (info = 0; argc > 1; --argc, ++argv) {
		if (argv[1][0] == '-') {
			for (p = &argv[1][1]; *p; ++p) {
				switch (*p) {
					case 'a': 
						info |= ALL;
						break;
					case 'm':
						info |= U_MACHINE;
						break;
					case 'n':
						info |= NODE_NAME;
						break;
					case 'r':
						info |= RELEASE;
						break;
					case 's':
						info |= SYS_NAME;
						break;
					case 'v':
						info |= VERSION;
						break;
					case 'p':
						info |= ARCH;
						break;
					default:
						usageErr();
				}
			}
		} else {
			usageErr();
		}
	}

	if (info == 0)
	  info = strcmp(prog, "arch") == 0 ? ARCH : SYS_NAME;

	if (uname(&un) != 0) {
		print(STDERR_FILENO, "unable to determine uname values\n", (char *) NULL);
		exit(EXIT_FAILURE);
	}

	if ((info & SYS_NAME) != 0) {
		print(STDOUT_FILENO, un.sysname, (char *) NULL);
		++i;
	}
	if ((info & NODE_NAME) != 0) {
		if (i++ > 0)
		  print(STDOUT_FILENO, " ", (char *) NULL);
		print(STDOUT_FILENO, un.nodename, (char *) NULL);
	}
	if ((info & RELEASE) != 0) {
		if (i++ > 0)
		  print(STDOUT_FILENO, " ", (char *) NULL);
		print(STDOUT_FILENO, un.release, (char *) NULL);
	}
	if ((info & VERSION) != 0) {
		if (i++ > 0)
		  print(STDOUT_FILENO, " ", (char *) NULL);
		print(STDOUT_FILENO, un.version, (char *) NULL);
	}
	if ((info & U_MACHINE) != 0) {
		if (i++ > 0)
		  print(STDOUT_FILENO, " ", (char *) NULL);
		print(STDOUT_FILENO, un.machine, (char *) NULL);
	}
	if ((info & ARCH) != 0) {
		if (i++ > 0)
		  print(STDOUT_FILENO, " ", (char *) NULL);
		print(STDOUT_FILENO, un.arch, (char *) NULL);
	}
	print(STDOUT_FILENO, "\n", (char *) NULL);
	return EXIT_SUCCESS;
}

