#include <fcntl.h>
#include "tlpi_hdr.h"

#define BUF_LEN 440

int main(int argc, char *argv[]) {
	if (argc < 3 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	  usageErr("%s device masterboot\n", argv[0]);

	int rawFd, mbrFd;
	char buf[BUF_LEN];
	memset(buf, 0, BUF_LEN);

	mbrFd = open(argv[2], O_RDONLY);
	if (mbrFd == -1)
	  errExit("open mbr");

	if (read(mbrFd, buf, BUF_LEN) < 0)
	  errExit("read");

	rawFd = open(argv[1], O_WRONLY);
	if (rawFd == -1)
	  errExit("open device");

	if (write(rawFd, buf, BUF_LEN) != BUF_LEN)
	  errExit("write");

	printf("Copy mbr to device successfully.\n");

	close(mbrFd);
	close(rawFd);

	exit(EXIT_SUCCESS);
}
