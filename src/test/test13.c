#include "util.h"

#define BLOCK_SIZE	1000
#define NUM_BLOCKS	1000

static char buffer[BLOCK_SIZE];

void main(int argc, char **argv) {
	int status, pipeFd[2];
	register int i;
	
	setup(argc, argv);

	pipe(pipeFd);

	switch (fork()) {
		case 0:
			/* Child code */
			for (i = 0; i < NUM_BLOCKS; ++i) {
				if (read(pipeFd[0], buffer, BLOCK_SIZE) != BLOCK_SIZE)
				  break;
			}
			exit(0);
		case -1:
			forkFailed();
		default:
			/* Parent code */
			for (i = 0; i < NUM_BLOCKS; ++i) {
				write(pipeFd[1], buffer, BLOCK_SIZE);
			}
			wait(&status);
			break;
	}

	quit();
}
