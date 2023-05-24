#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <minix/minlib.h>

int main(int argc, char **argv) {
	register int seconds;
	register char c, *s;

	if (argc != 2) {
		stdErr("Usage: sleep time\n");
		exit(1);
	}
	seconds = 0;
	s = argv[1];
	while ((c = *s++)) {
		if (c < '0' || c > '9') {
			stdErr("sleep: bad arg\n");
			exit(1);
		}
		seconds = 10 * seconds + (c - '0');
	}

	/* Now sleep. */
	sleep(seconds);
	return 0;
}
