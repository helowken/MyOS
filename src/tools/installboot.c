#include "common.h"
#include "image.h"
#include "../boot/rawfs.h"

#define MASTER_BOOT_OFF		0			/* Master boot offset in device */
#define MASTER_BOOT_SIZE	440
#define BOOT_BLOCK_OFF		0			/* Boot block offset in partition */
#define BOOT_BLOCK_SIZE		1024		/* Like standard UNIX, reserves 1K block of disk device
										   as a bootBlock, but only one 512-byte sector is loaded
										   by the master boot sector, so 512 bytes are available
										   for saving settings (params) */
#define PARAM_SIZE			512
#define SIGNATURE_POS		510
#define SIGNATURE			0xAA55
#define BOOT_MAX			64			/* bootable max size must be <= 64K, since BIOS has a 
										   DMA 64K boundary on loading data from disk. 
										   See biosDiskError(0x09) in boot.c. */
#define STACK_SIZE			0x1400		/* Stack and heap size. (since malloc() is allocate memory 
										   on stack when booting).
										   It should be aligned to the STACK_SIZE in boothead.s. */
#define DEVICE				"c0d0p0s0"
#define ALIGN(n)			(((n) + ((SECTOR_SIZE) - 1)) & ~((SECTOR_SIZE) - 1))

#define sDev(off)		Lseek(device, deviceFd, (off))
#define sPart(off)		sDev(OFFSET(lowSector) + (off))
#define rDev(buf,size)	Read(device, deviceFd, (buf), (size))
#define wDev(buf,size)	Write(device, deviceFd, (buf), (size)) 

static char *procName;
static char *device;
static int deviceFd;
static uint32_t lowSector;

static char *paramsTpl = 
	"rootdev=%s;"
	"ramimagedev=%s;"
	"%s"
	"minix(1,Start MINIX 3 (requires at least 16 MB RAM)) { "
	//	"unset image; "
		"boot; "
	"};"
	"main() { "
		"echo By default, MINIX 3 will automatically load in 3 seconds.; "
		"echo Press ESC to enter the monitor for special configuration.; "
		//"trap 3000 boot; "   Bochs may have a bug on system timer
		"menu; "
	"};";

static char *imgTpl = "image=%d:%d;";

static void usage() {
	fprintf(stderr,
	  "Usage: installboot -m(aster) device masterboot\n"
	  "       installboot -d(evice) device bootBlock boot\n");
	exit(1);
}

static uint32_t getLowSector(int deviceFd) {
	PartitionEntry pe;

	getActivePartition(deviceFd, &pe);
	return pe.lowSector;
}

typedef void (*preCopyFunc)(int destFd, int srcFd);

static void installMasterboot(char *masterboot) {
/* Install masterboot to the first sector of device */
	int mbrFd;
	char buf[MASTER_BOOT_SIZE];

	memset(buf, 0, MASTER_BOOT_SIZE);
	mbrFd = ROpen(masterboot);
	Read(masterboot, mbrFd, buf, MASTER_BOOT_SIZE);
	sDev(MASTER_BOOT_OFF);
	wDev(buf, MASTER_BOOT_SIZE);
	Close(masterboot, mbrFd);
}

static void checkElfHeader(char *fileName, Elf32_Ehdr *ehdrPtr) {
	if (ehdrPtr->e_ident[EI_MAG0] != ELFMAG0 ||
				ehdrPtr->e_ident[EI_MAG1] != ELFMAG1 ||
				ehdrPtr->e_ident[EI_MAG2] != ELFMAG2 ||
				ehdrPtr->e_ident[EI_MAG3] != ELFMAG3)
	  fatal("%s is not an ELF file.\n", fileName);
	
	if (ehdrPtr->e_ident[EI_CLASS] != ELFCLASS32)
	  fatal("%s is not an 32-bit executable.\n", fileName);

	if (ehdrPtr->e_ident[EI_DATA] != ELFDATA2LSB)
	  fatal("%s is not little endian.\n", fileName);

	if (ehdrPtr->e_ident[EI_VERSION] != EV_CURRENT)
	  fatal("%s has an invalid version.\n", fileName);

	if (ehdrPtr->e_type != ET_EXEC)
	  fatal("%s is not an executable.\n", fileName);
}

static long totalText = 0, totalData = 0, totalBss = 0;

static size_t readHeader(char *fileName, FILE *imgFile, ImageHeader *imgHdr, bool print) {
	int i, sn = 0;
	static bool banner = false;
	Exec *proc = &imgHdr->process;
	Elf32_Ehdr *ehdrPtr = &proc->ehdr;
	Elf32_Phdr phdr;
	size_t phdrSize, textSize = 0, dataSize = 0, bssSize = 0;
	size_t sizeInMemory = 0;

	memset(imgHdr, 0, sizeof(*imgHdr));
	strncpy(imgHdr->name, fileName, IMG_NAME_MAX);

	if (fread(ehdrPtr, sizeof(*ehdrPtr), 1, imgFile) != 1 || 
				ferror(imgFile))
	  errExit("fread elf header");

	checkElfHeader(fileName, ehdrPtr);

	if (fseek(imgFile, ehdrPtr->e_phoff, SEEK_SET) != 0)
	  errExit("fseek");

	phdrSize = ehdrPtr->e_phentsize;	
	for (i = 0; i < ehdrPtr->e_phnum; ++i) {
		if (fread(&phdr, phdrSize, 1, imgFile) != 1 || 
					ferror(imgFile))
		  errExit("fread program headers");

		if (isPLoad(&phdr)) {
			if (isRX(&phdr)) {
				memcpy(&proc->codeHdr, &phdr, phdrSize);
				textSize = phdr.p_filesz;
				sizeInMemory = phdr.p_vaddr + phdr.p_memsz;
				++sn;
			} else if (isRW(&phdr)) {
				memcpy(&proc->dataHdr, &phdr, phdrSize);
				dataSize = phdr.p_filesz;
				bssSize = phdr.p_memsz - dataSize;
				sizeInMemory = phdr.p_vaddr + phdr.p_memsz;
				++sn;
			}
			if (sn >= 2)
			  break;
		}
	}

	if (print) {
		if (!banner) {
			printf("     text     data      bss     size\n");
			banner = true;
		}
		printf(" %8d %8d %8d %8d	%s\n", textSize, dataSize, bssSize, 
					textSize + dataSize + bssSize, imgHdr->name);

		totalText += textSize;
		totalData += dataSize;
		totalBss += bssSize;
	}
	
	return sizeInMemory;
}

static char *imgBuf = NULL;
static size_t bufLen = 512;
static off_t bufOff = 0;

#define currBuf	(imgBuf + bufOff)

static void adjustBuf(size_t len) {
	if (bufOff + len > bufLen) {
		while ((bufLen *= 2) < bufOff + len) {
		}
		imgBuf = realloc(imgBuf, bufLen);
		memset(currBuf, 0, bufLen - bufOff);
	}
}

static void bwrite(void *srcBuf, size_t len) {
	adjustBuf(len);
	memcpy(currBuf, srcBuf, len);
	bufOff += len;
}

static void padImage(size_t len) {
	adjustBuf(len);
	memset(currBuf, 0, len);
	bufOff += len;
}

static void copyExec(char *procName, FILE *imgFile, Elf32_Phdr *hdr) {
	size_t size = hdr->p_filesz;
	int padLen;

	if (size == 0)
	  return;
	
	padLen = ALIGN(size) - size;

	adjustBuf(size);

	if (fseek(imgFile, hdr->p_offset, SEEK_SET) == -1)
	  errExit("fseek %s", procName);
	if (fread(currBuf, size, 1, imgFile) != 1 || ferror(imgFile))
	  errExit("copyExec fread %s", procName);

	bufOff += size;

	padImage(padLen);
}

FILE *openFile(char *file) {
	FILE *imgFile;
	struct stat st;

	if (stat(file, &st) < 0)
	  errExit("stat \"%s\"", file);
	if (!S_ISREG(st.st_mode))
	  errExit("\"%s\" is not a file.", file);
	if ((imgFile = fopen(file, "r")) == NULL)
	  errExit("fopen \"%s\"", file);
	return imgFile;
}

static void installImages(char **imgNames, int imgAddr, off_t bootSize, off_t *imgSizePtr) {
	FILE *imgFile;
	char *imgName, *file;
	imgBuf = malloc(bufLen);
	ImageHeader imgHdr;
	Exec *proc;

	while ((imgName = *imgNames++) != NULL) {
		if ((file = strrchr(imgName, ':')) != NULL)
		  ++file;
		else
		  file = imgName;

		imgFile = openFile(file);
		/* Use on sector to store exec header */
		readHeader(imgName, imgFile, &imgHdr, true);
		bwrite(&imgHdr, sizeof(imgHdr));
		padImage(SECTOR_SIZE - sizeof(imgHdr));
		
		proc = &imgHdr.process;
		if (isPLoad(&proc->codeHdr))
		  copyExec(imgName, imgFile, &proc->codeHdr);
		if (isPLoad(&proc->dataHdr))
		  copyExec(imgName, imgFile, &proc->dataHdr);
		
		fclose(imgFile);

		if (bootSize + bufOff > BOOT_IMG_SIZE)
		  fatal("Total size of (boot + images) excceeds %dMB", 
					  (BOOT_IMG_SIZE >> KB_SHIFT) >> KB_SHIFT);
	}
	printf("   ------   ------   ------   ------\n");
	printf(" %8ld %8ld %8ld %8ld	total\n", totalText, totalData, totalBss, 
				totalText + totalData + totalBss);

	sPart(OFFSET(imgAddr));
	wDev(imgBuf, bufOff);
	free(imgBuf);

	*imgSizePtr = bufOff;
}

static void checkBootMemSize(char *boot) {
	FILE *bootFile;
	ImageHeader imgHdr;
	size_t size;
	char bootElf[20];
	char *s;
	int i, n;

	s = strchr(boot, '.');
	n = (s == NULL) ? strlen(boot) : s - boot;

	for (i = 0; i < n; ++i) {
		bootElf[i] = boot[i];
	}
	bootElf[i++] = '.';
	bootElf[i++] = 'e';
	bootElf[i++] = 'l';
	bootElf[i++] = 'f';
	bootElf[i] = 0;

	bootFile = openFile(bootElf);
	size = readHeader(bootElf, bootFile, &imgHdr, false);
	printf("size: %x\n", size);
	printf("size: %x\n", size + STACK_SIZE);
	if (size + STACK_SIZE > (BOOT_MAX << KB_SHIFT))
	  fatal("Boot size > %d KB.", BOOT_MAX);
}

static void installBoot(char *boot, off_t *bootSizePtr, int *bootAddrPtr) {
	char *bootBuf;
	int bootFd;
	Off_t fsSize;		/* Total blocks of the filesystem */
	int blockSize = 0;
	off_t bootSize;
	int bootAddr;

	/* Read and check the superblock. */
	fsSize = rawSuper(&blockSize);
	if (fsSize == 0)
	  fatal("%s: %s is not a Minix file system\n", procName, device);

	/* Calculate boot size and addr. */
	checkBootMemSize(boot);
	bootSize = getFileSize(boot);
	bootAddr = fsSize * RATIO(blockSize);

	/* Read boot to buf. */
	bootBuf = (char *) malloc(bootSize);
	if (bootBuf == NULL)
	  errExit("malloc boot buf");
	bootFd = ROpen(boot);
	Read(boot, bootFd, bootBuf, bootSize);

	/* Write boot buf to device. */
	sPart(OFFSET(bootAddr));
	wDev(bootBuf, bootSize);
	free(bootBuf);

	*bootSizePtr = bootSize;
	*bootAddrPtr = bootAddr;
}

static void installParams(char *params, int imgAddr, off_t imgSectors) {
#define IMG_STR_LEN	50
	char imgStr[IMG_STR_LEN];
	int n;

	if ((n = snprintf(imgStr, IMG_STR_LEN, imgTpl, imgAddr, imgSectors)) < 0)
	  errExit("create image param");
	imgStr[n] = 0;
	memset(params, ';', PARAM_SIZE);
	if (snprintf(params, PARAM_SIZE, paramsTpl, DEVICE, DEVICE, imgStr) < 0)
	  errExit("snprintf params");
}

static void installDevice(char *bootBlock, char *boot, char **imgNames) {
/* Install bootBlock to the boot sector with boot's disk addresses and sizes patched 
 * into the data segment of bootBlock. 
 */
	char buf[BOOT_BLOCK_SIZE];
	int bootBlockFd;
	ssize_t bootBlockSize;
	char *ap;		
	int bootAddr, imgAddr;
	off_t bootSize, imgSize;
	int bootSectors, imgSectors;

	/* Install boot to device. */
	installBoot(boot, &bootSize, &bootAddr);
	bootSectors = SECTORS(bootSize);
	imgAddr = bootAddr + SECTORS(bootSize);

	/* Read bootBlock to buf. */
	memset(buf, 0, BOOT_BLOCK_SIZE);
	bootBlockFd = ROpen(bootBlock);
	bootBlockSize = Read(bootBlock, bootBlockFd, buf, BOOT_BLOCK_SIZE);
	Close(bootBlock, bootBlockFd);

	if (bootBlockSize + 4 >= SIGNATURE_POS)	/* 1 size byte + 3 address bytes (showned below) */
	  fatal("%s + addresses to %s don't fit in the boot sector.", bootBlock, boot);

	/* Patch boot's size and address into the data segment of bootBlock. 
	 * (See "boot/bootBlock.asm".)
	 */
	ap = &buf[bootBlockSize];
	*ap++ = bootSectors;		/* Boot sectors */
	*ap++ = bootAddr & 0xFF;			/* Boot addr [0..7] */
	*ap++ = (bootAddr >> 8) & 0xFF;		/* Boot addr [8..15] */
	*ap++ = (bootAddr >> 16) & 0xFF;	/* Boot addr [16..23] */

	/* The last two bytes is signature. */
	buf[SIGNATURE_POS] = SIGNATURE & 0xFF;
	buf[SIGNATURE_POS + 1] = (SIGNATURE >> 8) & 0xFF;


	/* Install images to device. */
	printf("\n");
	installImages(imgNames, imgAddr, bootSize, &imgSize);
	printf("\n");
	imgSectors = SECTORS(imgSize);

	/* Install params. */
	installParams(&buf[SECTOR_SIZE], imgAddr, imgSectors);

	/* Write bootBlock to device. */
	sPart(BOOT_BLOCK_OFF);
	wDev(buf, BOOT_BLOCK_SIZE);

	printf("Boot addr: %d, sectors: %d\n", bootAddr, bootSectors);
	printf("Image addr: %d, sectors: %d\n", imgAddr, imgSectors);
}

void readBlock(Off_t blockNum, char *buf, int blockSize) {
/* For rawfs, so that it can read blocks. */
	sPart(blockNum * blockSize);
	rDev(buf, blockSize);
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

	procName = argv[0];
	device = argv[2];
	deviceFd = RWOpen(device);
	lowSector = getLowSector(deviceFd);

	if (isOpt(argv[1], "-master")) {
	    installMasterboot(argv[3]);
	} else if (argc >= 6 && isOpt(argv[1], "-device")) {
	    installDevice(argv[3], argv[4], argv + 5);		
	} else {
	  usage();
	}

	Close(device, deviceFd);

	exit(EXIT_SUCCESS);
}
