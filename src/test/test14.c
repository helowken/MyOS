#include "util.h"

#define TRIALS	100

static char name[20] = "TMP13.";

void main(int argc, char **argv) {
	int fd, i, pid;

	setup(argc, argv);

	pid = getpid();
	name[6] = (pid & 037) + 33;
	name[7] = ((pid * pid) & 037) + 33;
	name[8] = 0;

	for (i = 0; i < TRIALS; ++i) {
		if ((fd = creat(name, 0777)) < 0) e(1);
		if (write(fd, name, 20) != 20) e(2);
		if (unlink(name) != 0) e(3);
		if (close(fd) != 0) e(4);
	}

	fd = creat(name, 0777);
	write(fd, name, 20);
	unlink(name);

	quit();
}
