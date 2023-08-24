#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <minix/minlib.h>
#include <minix/const.h>
#include <errno.h>
#include <stdio.h>

static char *prog;

static void usageErr() {
	usage(prog, "name b/c/p [major minor]");
}

int main(int argc, char **argv) {
/* Mknod name b/c major minor makes a node. */
	int mode, major, minor, dev;

	prog = getProg(argv);
	if (argc < 3) usageErr();
	if (*argv[2] != 'b' && *argv[2] != 'c' && *argv[2] != 'p') usageErr();
	if (*argv[2] == 'p' && argc != 3) usage(prog, "name p");
	if (*argv[2] == 'c' && argc != 5) usage(prog, "name c major minor");
	if (*argv[2] == 'b' && argc != 5) usage(prog, "name b major minor");
	if (*argv[2] == 'p') {
		mode = I_NAMED_PIPE | 0666;
		dev = 0;
	} else {
		mode = (*argv[2] == 'b' ? I_BLOCK_SPECIAL | 0666 : I_CHAR_SPECIAL | 0666);
		major = atoi(argv[3]);
		minor = atoi(argv[4]);
		if (major - 1 > 0xFE || minor > 0xFF) usageErr();
		dev = (major << 8) | minor;
	}
	if (mknod(argv[1], mode, dev) < 0) {
		reportStdErr(prog, argv[1]);
		return 1;
	}
	return 0;
}
