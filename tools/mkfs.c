#include <time.h>
#include "common.h"
#include "mkfs.h"

static unsigned int blockSize;
static int inodesPerBlock;
static char *zero;

static void usage(char *s) {
	fprintf(stderr, "Usage: %s device\n", s);
	exit(1);
}

static char *allocBlock() {
	char *buf;

	if (!(buf = malloc(blockSize)))
	  errExit("Couldn't allocate file system buffer");
	memset(buf, 0, blockSize);

	return buf;
}

static block_t sizeUp(char *device) {
	int fd;
	struct stat st;
	block_t d;

	if ((fd = open(device, O_RDONLY)) == -1) 
	  errExit("sizeup open");
	if (fstat(fd, &st) < 0) 
	  errExit("fstat failed");

	d = st.st_size / blockSize;

	close(fd);
	return d;
}

int main(int argc, char *argv[]) {
	char *device;
	block_t blocks, maxBlocks;

	if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	  usage(argv[0]);

	device = argv[0];
	blockSize = MAX_BLOCK_SIZE;	
	inodesPerBlock = INODES_PER_BLOCK(blockSize);

	zero = allocBlock();

	maxBlocks = sizeUp(device);

	return EXIT_SUCCESS;
}
