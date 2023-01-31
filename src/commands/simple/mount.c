#include "errno.h"
#include "sys/types.h"
#include "limits.h"
#include "stdlib.h"
#include "dirent.h"
#include "string.h"
#include "unistd.h"
#include "fcntl.h"
#include "minix/config.h"
#include "minix/const.h"
#include "minix/minlib.h"
#include "minix/swap.h"
#include "sys/svrctl.h"
#include "stdio.h"
#include "../../servers/fs/const.h"

//static u8_t MAGIC[] = { SWAP_MAGIC0, SWAP_MAGIC1, SWAP_MAGIC2, SWAP_MAGIC3 };

static void usageErr() {
	stdErr("Usage: mount [-r] special file\n"
		   "       mount -s swapfile\n");
	exit(1);
}

static void printMount(char *special, char *mountedOn, bool readonly) {
	tell(special);
	tell(" is read-");
	tell(readonly ? "only" : "write");
	tell(" mounted on ");
	tell(mountedOn);
	tell("\n");
}

static void list() {
	char special[PATH_MAX + 1], mountedOn[PATH_MAX + 1], version[10], rwFlag[10];

	/* Read and print /etc/mtab. */
	if (loadMTab("mount") < 0)
	  exit(1);

	while (1) {
		if (getMTabEntry(special, mountedOn, version, rwFlag) < 0)
		  break;
		if (strcmp(version, "swap") == 0) {
			tell(special);
			tell(" is swapspace\n");
		} else {
			printMount(special, mountedOn, strcmp(rwFlag, "rw") == 0);
		}
	}
	exit(0);
}

static void swapOn(char *file) {
	stdErr("=== TODO swapOn mount\n");
	exit(1);
}

int main(int argc, char **argv) {
	int i, readonly, swap, v;
	char **ap, *opt, *err, *vs;
	char special[PATH_MAX + 1], mountedOn[PATH_MAX + 1], version[10], rwFlag[10];

	if (argc == 1)
	  list();	/* Just list /etc/mtab */

	readonly = 0;
	swap = 0;
	ap = argv + 1;
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			opt = argv[i] + 1;
			while (*opt != 0) {
				switch (*opt++) {
					case 'r':
						readonly = 1;
						break;
					case 's':
						swap = 1;
						break;
					default:
						usageErr();
				}
			}
		} else {
			*ap++ = argv[i];
		}
	}
	*ap = NULL;
	argc = (ap - argv);

	if (readonly && swap)
	  usageErr();

	if (swap) {
		if (argc != 2)
		  usageErr();
		swapOn(argv[1]);
		tell(argv[1]);
		tell(" is swapspace\n");
	} else {
		if (argc != 3)
		  usageErr();
		if (mount(argv[1], argv[2], readonly) < 0) {
			err = strerror(errno);
			stdErr("mount: Can't mount ");
			stdErr(argv[1]);
			stdErr(" on ");
			stdErr(argv[2]);
			stdErr(": ");
			stdErr(err);
			stdErr("\n");
			exit(1);
		}
		/* The mount has completed successfully. Tell the user. */
		printMount(argv[1], argv[2], readonly);
	}

	/* Update /etc/mtab. */
	if (loadMTab("mount") < 0)
	  exit(1);

	/* Loop on all the /etc/mtab entries, copying each one to the output buf. */
	while (1) {
		if (getMTabEntry(special, mountedOn, version, rwFlag) < 0)
		  break;
		if (putMTabEntry(special, mountedOn, version, rwFlag) < 0) {
			stdErr("mount: /etc/mtab has grown too large.\n");
			exit(1);
		}
	}
	if (swap) {
		vs = "swap";		
	} else {
		v = fsVersion(argv[1], "mount");
		if (v == 3)
		  vs = "3";
		else
		  vs = "0";
	}
	if (putMTabEntry(argv[1], swap ? "swap" : argv[2], vs, 
				readonly ? "ro" : "rw") < 0) {
		stdErr("mount: /etc/mtab has grown too large.\n");
		exit(1);
	}
	rewriteMTab("mount");
	return 0;
}
