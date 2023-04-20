#define _MINIX	1
#define	_POSIX_SOURCE	1

#include "sys/types.h"
#include "sys/svrctl.h"
#include "fcntl.h"
#include "errno.h"
#include "limits.h"
#include "minix/minlib.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "stdio.h"

static char *prog;
static char mountPoint[PATH_MAX + 1];

static void usageErr() {
	usage(prog, "[-s] special");
}

static void updateMTab(char *devName) {
	char special[PATH_MAX + 1], mountedOn[PATH_MAX + 1], version[10], rwFlag[10];

	if (loadMTab(prog) < 0) {
		stdErr("/etc/mtab not updated.\n");
		exit(1);
	}
	while (1) {
		if (getMTabEntry(special, mountedOn, version, rwFlag) < 0)
		  break;
		if (strcmp(devName, special) == 0) {
			strcpy(mountPoint, mountedOn);
			continue;
		}
		putMTabEntry(special, mountedOn, version, rwFlag);
	}
	if (rewriteMTab(prog) < 0) {
		stdErr("/etc/mtab not updated.\n");
		exit(1);
	}
}

int main(int argc, char **argv) {
	int sflag = 0;	

	prog = getProg(argv);
	while (argc > 1 && argv[1][0] == '-') {
		char *opt = argv[1] + 1;
		while (*opt) {
			if (*opt++ == 's')
			  sflag = 1;
			else
			  usageErr();
		}
		--argc;
		++argv;
	}
	if (argc != 2)
	  usageErr();
	if ((sflag ? svrctl(MM_SWAP_OFF, NULL) : umount(argv[1])) < 0) {
		if (errno == EINVAL)
		  stdErr("Device not mounted\n");
		else
		  perror(prog);
		exit(1);
	}
	updateMTab(argv[1]);
	tell(argv[1]);
	tell(" unmounted");
	if (*mountPoint != '\0') {
		tell(" from ");
		tell(mountPoint);
	}
	tell("\n");
	return 0;
}
