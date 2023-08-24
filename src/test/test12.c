#include "util.h"

#define NUM_TIMES	1000

void main(int argc, char **argv) {
	register int i;
	int k;

	setup(argc, argv);

	for (i = 0; i < NUM_TIMES; ++i) {
		switch (fork()) {
			case 0: 
				exit(1);
			case -1: 
				forkFailed();
			default:
				wait(&k);
				break;
		}
	}

	quit();
}
