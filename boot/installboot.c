#include <fcntl.h>
#include <sys/stat.h>
#include "tlpi_hdr.h"

#define MASTER_BOOT_LEN		440
#define BOOT_BLOCK_LEN		512
#define SIGNATURE_POS		510
#define SIGNATURE			0XAA55
#define SECTOR_SIZE			512
#define TESTBOOT_SEC_OFF	8
#define TESTBOOT_SEC_COUNT	1

static void usage() {
	fprintf(stderr,
	  "Usage: installboot -m(aster) device masterboot\n"
	  "		  installboot -b(ootable) device bootblock\n");
	exit(1);
}

typedef void (*preCopyFunc)(int destFd, int srcFd);

static void copyTo(char *dest, char *src, char *buf, int len, preCopyFunc func) {
	int destFd, srcFd;

	srcFd = open(src, O_RDONLY);
	if (srcFd == -1)
	  errExit("open src");

	destFd = open(dest, O_WRONLY);
	if (destFd == -1)
	  errExit("open dest");

	if (func != NULL)
	  func(destFd, srcFd);

	if (read(srcFd, buf, len) < 0)
	  errExit("read");

	if (write(destFd, buf, len) != len)
	  errExit("write");

	close(srcFd);
	close(destFd);
}

static off_t getFileSize(char *pathName) {
	struct stat sb;
	if (stat(pathName, &sb) == -1)
	  errExit("stat");
	return sb.st_size;
}

static void install_masterboot(char *device, char *masterboot) {
	int len = MASTER_BOOT_LEN;
	char buf[len];
	memset(buf, 0, len);
	copyTo(device, masterboot, buf, len, NULL);
	printf("Install %s to %s successfully.\n", masterboot, device);
}

static void install_bootable(char *device, char *bootblock) {
	int addr = TESTBOOT_SEC_OFF;
	int len = BOOT_BLOCK_LEN;
	char buf[len];
	memset(buf, 0, len);
	buf[SIGNATURE_POS] = SIGNATURE & 0xFF;
	buf[SIGNATURE_POS + 1] = (SIGNATURE >> 8) & 0xFF;

	off_t size = getFileSize(bootblock);
	char *ap = &buf[size];
	*ap++ = TESTBOOT_SEC_COUNT;
	*ap++ = addr & 0xFF;
	*ap++ = (addr >> 8) & 0xFF;
	*ap++ = (addr >> 16) & 0xFF;

	copyTo(device, bootblock, buf, len - 2, NULL);
	printf("Install %s to %s successfully.\n", bootblock, device);
}

static void install_testBoot(char *device, char *testBoot) {
	int len = SECTOR_SIZE * TESTBOOT_SEC_COUNT;
	char buf[len];
	memset(buf, 0, len);

	void func(int destFd, int srcFd) {
		off_t pos = SECTOR_SIZE * TESTBOOT_SEC_OFF;
		if (lseek(destFd, pos, SEEK_SET) == -1)
		  errExit("lseek dest fd");
	}

	copyTo(device, testBoot, buf, len, func);
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
	else if (isOpt(argv[1], "-testBoot"))
	  install_testBoot(argv[2], argv[3]);
	else 
	  usage();

	exit(EXIT_SUCCESS);
}
