#include <fcntl.h>
#include "tlpi_hdr.h"

#define MASTER_BOOT_LEN 440
#define BOOT_BLOCK_LEN	512
#define SIGNATURE_POS	510
#define SIGNATURE		0XAA55


static void usage() {
	fprintf(stderr,
	  "Usage: installboot -m(aster) device masterboot\n"
	  "		  installboot -b(ootable) device bootblock\n");
	exit(1);
}

static void copyTo(char *dest, char *src, char *buf, int len) {
	int destFd, srcFd;

	srcFd = open(src, O_RDONLY);
	if (srcFd == -1)
	  errExit("open src");

	if (read(srcFd, buf, len) < 0)
	  errExit("read");

	destFd = open(dest, O_WRONLY);
	if (destFd == -1)
	  errExit("open dest");

	if (write(destFd, buf, len) != len)
	  errExit("write");

	close(srcFd);
	close(destFd);
}

static void install_masterboot(char *device, char *masterboot) {
	int len = MASTER_BOOT_LEN;
	char buf[len];
	memset(buf, 0, len);
	copyTo(device, masterboot, buf, len);
	printf("Install %s to %s successfully.\n", masterboot, device);
}

static void install_bootable(char *device, char *bootblock) {
	int len = BOOT_BLOCK_LEN;
	char buf[len];
	memset(buf, 0, len);
	buf[SIGNATURE_POS] = SIGNATURE & 0xFF;
	buf[SIGNATURE_POS + 1] = (SIGNATURE >> 8) & 0xFF;
	copyTo(device, bootblock, buf, len - 2);
	printf("Install %s to %s successfully.\n", bootblock, device);
}

static Boolean isOpt(char *opt, char *test) {
	if (strcmp(opt, test) == 0) return TRUE;
	if (opt[0] != '-' || strlen(opt) != 2) return FALSE;
	if (opt[1] == test[1]) return TRUE;
	return FALSE;
}

int main(int argc, char *argv[]) {
	if (argc < 4 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	  usage();

	if (isOpt(argv[1], "-master"))
	  install_masterboot(argv[2], argv[3]);
	else if (isOpt(argv[1], "-bootable"))
	  install_bootable(argv[2], argv[3]);

	exit(EXIT_SUCCESS);
}
