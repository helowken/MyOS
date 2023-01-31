#include "sys/types.h"
#include "sys/svrctl.h"
#include "stdarg.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"
#include "string.h"
#include "minix/minlib.h"

static char *prog;

int main(int argc, char **argv) {
	SysGetEnv sysGetEnv;
	int i;
	int ex = 0;
	char *e;
	char val[1024];
	char *opt;

	prog = argv[0];
	i = 1;
	while (i < argc && argv[i][0] == '-') {
		opt = argv[i++] + 1;
		if (opt[0] == '-' && opt[1] == 0)	/* -- */
		  break;

		if (*opt != 0) 
		  usage(prog, "[name ...]");
	}

	do {
		if (i < argc) {
			sysGetEnv.key = argv[i];
			sysGetEnv.keyLen = strlen(sysGetEnv.key) + 1;
		} else {
			sysGetEnv.key = NULL;
			sysGetEnv.keyLen = 0;
		}
		sysGetEnv.val = val;
		sysGetEnv.valLen = sizeof(val);

		if (svrctl(MM_GET_PARAM, &sysGetEnv) == -1) {
			if (errno == ESRCH) {
				ex |= 2;
			} else {
				ex |= 1;
				report(prog, NULL);
			}
			continue;
		}

		/* if get all params, the val will be "k1=v1\0k2=v2\0". */
		e = sysGetEnv.val;
		do {
			e += strlen(e);
			*e++ = '\n';
		} while (i == argc && *e != 0);

		if (write(STDOUT_FILENO, sysGetEnv.val, e - sysGetEnv.val) < 0) {
			ex |= 1;
			report(prog, NULL);
		}
	} while (++i < argc);

	return ex;
}
