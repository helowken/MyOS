#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <elf.h>
#include "tlpi_hdr.h"

#define MASTER_BOOT_LEN		440
#define BOOT_BLOCK_LEN		512
#define PARAM_LEN			512
#define SIGNATURE_POS		510
#define SIGNATURE			0XAA55
#define SECTOR_SIZE			512
#define BOOT_MAX_SECTORS	80			/* bootable max size must be <= 64K */
#define BOOT_SEC_OFF		8
#define	BOOT_STACK_SIZE		0x2800		/* Assume boot code using 10K stack */
#define IMAGE_NAME_MAX	63

#define sectorCount(size)	((size + SECTOR_SIZE - 1) / SECTOR_SIZE)
#define isRX(p)				(((p)->p_flags & PF_R) && ((p)->p_flags & PF_X))
#define isRW(p)				(((p)->p_flags & PF_R) && ((p)->p_flags & PF_W))

typedef struct {
	char	name[IMAGE_NAME_MAX + 1];	/* Null terminated. */
	Elf32_Ehdr	ehdr;					/* ELF header */
	Elf32_Phdr	codeHdr;				/* Program header for text and rodata */
	Elf32_Phdr	dataHdr;				/* Program header for data and bss */
} ImageHeader;

static char *paramsTpl = 
	"rootdev=%s;"
	"ramimagedev=%s;"
	"minix(1,Start MINIX 3 (requires at least 16 MB RAM)) { "
		"unset image; boot; "
	"};"
	"main() { "
		"echo By default, MINIX 3 will automatically load in 3 seconds.; "
		"echo Press ESC to enter the monitor for special configuration.; "
		"trap 3000 boot; "
		"menu; "
	"};";


static void usage() {
	fprintf(stderr,
	  "Usage: installboot -m(aster) device masterboot\n"
	  "       installboot -i(mage) image kernel mm fs ... init\n"
	  "       installboot -d(evice) device bootblock boot\n"
	  "       installboot -b(ootable) device bootblock\n");
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

static void installMasterboot(char *device, char *masterboot) {
	int len = MASTER_BOOT_LEN;
	char buf[len];
	memset(buf, 0, len);
	copyTo(device, masterboot, buf, len, NULL);
	//printf("Install %s to %s successfully.\n", masterboot, device);
}

static void installDevice(char *device, char *bootblock, char *boot) {
	int addr = BOOT_SEC_OFF;
	int len = BOOT_BLOCK_LEN + PARAM_LEN;
	char buf[len];
	off_t size, bootSize;
	char *ap;

	memset(buf, 0, BOOT_BLOCK_LEN);
	memset(&buf[BOOT_BLOCK_LEN], ';', PARAM_LEN);
	buf[SIGNATURE_POS] = SIGNATURE & 0xFF;
	buf[SIGNATURE_POS + 1] = (SIGNATURE >> 8) & 0xFF;

	size = getFileSize(bootblock);
	bootSize = getFileSize(boot);
	ap = &buf[size];
	*ap++ = sectorCount(bootSize);
	*ap++ = addr & 0xFF;
	*ap++ = (addr >> 8) & 0xFF;
	*ap++ = (addr >> 16) & 0xFF;

	if (snprintf(&buf[BOOT_BLOCK_LEN], PARAM_LEN, paramsTpl, device, device) < 0)
		  errExit("snprintf params");

	copyTo(device, bootblock, buf, len - 2, NULL);
	//printf("Install %s to %s successfully.\n", bootblock, device);
}

static void installBootable(char *device, char *boot) {
	off_t size = getFileSize(boot) + BOOT_STACK_SIZE;
	int sectors = sectorCount(size);
	if (sectors > BOOT_MAX_SECTORS)
	  fatal("Bootable size > 64K.");
	int len = SECTOR_SIZE * sectors;
	char buf[len];
	memset(buf, 0, len);

	void func(int destFd, int srcFd) {
		off_t pos = SECTOR_SIZE * BOOT_SEC_OFF;
		if (lseek(destFd, pos, SEEK_SET) == -1)
		  errExit("lseek dest fd");
	}

	copyTo(device, boot, buf, len, func);
}

static void checkElfHeader(char *procName, Elf32_Ehdr *ehdrPtr) {
	if (ehdrPtr->e_ident[EI_MAG0] != ELFMAG0 ||
				ehdrPtr->e_ident[EI_MAG1] != ELFMAG1 ||
				ehdrPtr->e_ident[EI_MAG2] != ELFMAG2 ||
				ehdrPtr->e_ident[EI_MAG3] != ELFMAG3)
	  fatal("%s is not an ELF file.\n", procName);
	
	if (ehdrPtr->e_ident[EI_CLASS] != ELFCLASS32)
	  fatal("%s is not an 32-bit executable.\n", procName);

	if (ehdrPtr->e_ident[EI_DATA] != ELFDATA2LSB)
	  fatal("%s is not little endian.\n", procName);

	if (ehdrPtr->e_ident[EI_VERSION] != EV_CURRENT)
	  fatal("%s has an invalid version.\n", procName);

	if (ehdrPtr->e_type != ET_EXEC)
	  fatal("%s is not an executable.\n", procName);
}

/*
static void printProgramHeader(Elf32_Phdr *phdrPtr) {
	if (phdrPtr->p_type != PT_NULL) {
		printf("type: %d\n", phdrPtr->p_type);
		printf("offset: %d\n", phdrPtr->p_offset);
		printf("vaddr: %d\n", phdrPtr->p_vaddr);
		printf("paddr: %d\n", phdrPtr->p_paddr);
		printf("filesz: %d\n", phdrPtr->p_filesz);
		printf("memsz: %d\n", phdrPtr->p_memsz);
		printf("flags: %d\n", phdrPtr->p_flags);
		printf("type: %d\n", phdrPtr->p_align);
	}
}
*/

static long totalText = 0, totalData = 0, totalBss = 0;

static void readHeader(char *procName, FILE *procFile, ImageHeader *imgHdr) {
	int n, i, sn = 0;
	static bool banner = false;
	Elf32_Ehdr *ehdrPtr = &imgHdr->ehdr;
	Elf32_Phdr phdr;
	size_t phdrSize, textSize = 0, dataSize = 0, bssSize = 0;

	memset(imgHdr, 0, sizeof(*imgHdr));
	strncpy(imgHdr->name, procName, IMAGE_NAME_MAX);

	n = fread(ehdrPtr, sizeof(char), sizeof(*ehdrPtr), procFile);
	if (ferror(procFile) || n < sizeof(*ehdrPtr))
	  errExit("fread elf header");

	checkElfHeader(procName, ehdrPtr);

	if (fseek(procFile, ehdrPtr->e_phoff, SEEK_SET) != 0)
	  errExit("fseek");

	phdrSize = ehdrPtr->e_phentsize;	
	for (i = 0; i < ehdrPtr->e_phnum; ++i) {
		n = fread(&phdr, sizeof(char), phdrSize, procFile);
		if (ferror(procFile) || n < phdrSize)
		  errExit("fread program headers");

		if (phdr.p_type == PT_LOAD) {
			if (isRX(&phdr)) {
				memcpy(&imgHdr->codeHdr, &phdr, phdrSize);
				textSize = imgHdr->codeHdr.p_filesz;
				++sn;
			} else if (isRW(&phdr)) {
				memcpy(&imgHdr->dataHdr, &phdr, phdrSize);
				dataSize = imgHdr->dataHdr.p_filesz;
				bssSize = imgHdr->dataHdr.p_memsz - dataSize;
				++sn;
			}
			if (sn >= 2)
			  break;
		}
	}

	if (!banner) {
		printf("     text     data      bss     size\n");
		banner = true;
	}
	printf(" %8ld %8ld %8ld %8ld	%s\n", textSize, dataSize, bssSize, 
				textSize + dataSize + bssSize, imgHdr->name);

	totalText += textSize;
	totalData += dataSize;
	totalBss += bssSize;
}

static void makeImage(char *image, char **procNames) {
	FILE *imageFile, *procFile;
	char *procName, *file;
	struct stat st;
	ImageHeader imgHdr;

	if ((imageFile = fopen(image, "w")) == NULL)
	  errExit("fopen \"%s\"", image);

	while ((procName = *procNames++) != NULL) {
		if ((file = strrchr(procName, ':')) != NULL)
		  ++file;
		else
		  file = procName;

		if (stat(file, &st) < 0)
		  errExit("stat \"%s\"", file);
		if (!S_ISREG(st.st_mode))
		  errExit("\"%s\" is not a file.", file);
		if ((procFile = fopen(file, "r")) == NULL)
		  errExit("fopen \"%s\"", file);

		readHeader(procName, procFile, &imgHdr);
	}
}

static bool isOpt(char *opt, char *test) {
	if (strcmp(opt, test) == 0) return true;
	if (opt[0] != '-' || strlen(opt) != 2) return false;
	if (opt[1] == test[1]) return true;
	return false;
}

int main(int argc, char *argv[]) {
	if (argc < 4 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	  usage();

	if (isOpt(argv[1], "-master"))
	  installMasterboot(argv[2], argv[3]);
	else if (argc >= 4 && isOpt(argv[1], "-image"))
	  makeImage(argv[2], argv + 3);
	else if (argc >= 5 && isOpt(argv[1], "-device"))
	  installDevice(argv[2], argv[3], argv[4]);
	else if (isOpt(argv[1], "-bootable"))
	  installBootable(argv[2], argv[3]);
	else 
	  usage();

	exit(EXIT_SUCCESS);
}
