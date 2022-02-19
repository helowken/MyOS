#include "common.h"
#include "image.h"
#include "rawfs.h"

#define MASTER_BOOT_SIZE	440
#define BOOT_BLOCK_SIZE		512			/* Like standard UNIX, reserves 1K block of disk device
										   as a bootBlock, but only one 512-byte sector is loaded
										   by the master boot sector, so 512 bytes are available
										   for saving settings (params) */
#define PARAM_SIZE			512
#define SIGNATURE_POS		510
#define SIGNATURE			0xAA55
#define BOOT_MAX			64			/* bootable max size must be <= 64K, since BIOS has a 
										   DMA 64K boundary on loading data from disk. 
										   See biosDiskError(0x09) in boot.c. */

#define isRX(p)				(((p)->p_flags & PF_R) && ((p)->p_flags & PF_X))
#define isRW(p)				(((p)->p_flags & PF_R) && ((p)->p_flags & PF_W))
#define ALIGN(n)			(((n) + ((SECTOR_SIZE) - 1)) & ~((SECTOR_SIZE) - 1))

#define sDev(off)		Lseek(device, deviceFd, (off))
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
		"trap 3000 boot; "
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

static void readHeader(char *fileName, FILE *imgFile, ImageHeader *imgHdr) {
	int i, sn = 0;
	static bool banner = false;
	Exec *proc = &imgHdr->process;
	Elf32_Ehdr *ehdrPtr = &proc->ehdr;
	Elf32_Phdr phdr;
	size_t phdrSize, textSize = 0, dataSize = 0, bssSize = 0;

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
				textSize = proc->codeHdr.p_filesz;
				++sn;
			} else if (isRW(&phdr)) {
				memcpy(&proc->dataHdr, &phdr, phdrSize);
				dataSize = proc->dataHdr.p_filesz;
				bssSize = proc->dataHdr.p_memsz - dataSize;
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
	printf(" %8d %8d %8d %8d	%s\n", textSize, dataSize, bssSize, 
				textSize + dataSize + bssSize, imgHdr->name);

	totalText += textSize;
	totalData += dataSize;
	totalBss += bssSize;
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

static void installImages(char **imgNames, int pos, off_t currSize, off_t *imgSizePtr) {
	FILE *imgFile;
	char *imgName, *file;
	struct stat st;
	imgBuf = malloc(bufLen);
	ImageHeader imgHdr;
	Exec *proc;

	while ((imgName = *imgNames++) != NULL) {
		if ((file = strrchr(imgName, ':')) != NULL)
		  ++file;
		else
		  file = imgName;

		if (stat(file, &st) < 0)
		  errExit("stat \"%s\"", file);
		if (!S_ISREG(st.st_mode))
		  errExit("\"%s\" is not a file.", file);
		if ((imgFile = fopen(file, "r")) == NULL)
		  errExit("fopen \"%s\"", file);

		/* Use on sector to store exec header */
		readHeader(imgName, imgFile, &imgHdr);
		bwrite(&imgHdr, sizeof(imgHdr));
		padImage(SECTOR_SIZE - sizeof(imgHdr));
		
		proc = &imgHdr.process;
		if (isPLoad(&proc->codeHdr))
		  copyExec(imgName, imgFile, &proc->codeHdr);
		if (isPLoad(&proc->dataHdr))
		  copyExec(imgName, imgFile, &proc->dataHdr);
		
		fclose(imgFile);

		if (currSize + bufOff > BOOT_IMG_SIZE)
		  fatal("Total size of (boot + images) excceeds %dMB", 
					  (BOOT_IMG_SIZE >> KB_SHIFT) >> KB_SHIFT);
	}
	printf("   ------   ------   ------   ------\n");
	printf(" %8ld %8ld %8ld %8ld	total\n", totalText, totalData, totalBss, 
				totalText + totalData + totalBss);

	sDev(OFFSET(pos));
	wDev(imgBuf, bufOff);
	free(imgBuf);

	*imgSizePtr = bufOff;
}

static void installBoot(char *boot, off_t *bootSizePtr, int *bootAddrPtr, int *posPtr) {
	char *bootBuf;
	int bootFd;
	Off_t fsSize;		/* Total blocks of the filesystem */
	int blockSize = 0;
	off_t bootSize;
	int bootAddr, pos;

	/* Read and check the superblock. */
	fsSize = rawSuper(&blockSize);
	if (fsSize == 0)
	  fatal("%s: %s is not a Minix file system\n", procName, device);

	/* Calculate boot size and addr. */
	bootSize = getFileSize(boot);
	if (bootSize > (BOOT_MAX << KB_SHIFT))
	  fatal("Bootable size > %d KB.", BOOT_MAX);
	bootAddr = fsSize * RATIO(blockSize);

	/* Read boot to buf. */
	bootBuf = (char *) malloc(bootSize);
	if (bootBuf == NULL)
	  errExit("malloc boot buf");
	bootFd = ROpen(boot);
	Read(boot, bootFd, bootBuf, bootSize);

	/* Write boot buf to device. */
	pos = lowSector + bootAddr;
	sDev(OFFSET(pos));
	wDev(bootBuf, bootSize);
	free(bootBuf);

	*bootSizePtr = bootSize;
	*bootAddrPtr = bootAddr;
	*posPtr = pos;
}

static void installParams(char *params, int pos, off_t imgSize) {
#define IMG_STR_LEN	50
	char imgStr[IMG_STR_LEN];
	int n;

	if ((n = snprintf(imgStr, IMG_STR_LEN, imgTpl, pos, SECTORS(imgSize))) < 0)
	  fatal("create image param");
	imgStr[n] = 0;
	memset(params, ';', PARAM_SIZE);
	if (snprintf(params, PARAM_SIZE, paramsTpl, device, device, imgStr) < 0)
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
	int pos, bootAddr;
	off_t bootSize, imgSize;

	/* Install boot to device. */
	installBoot(boot, &bootSize, &bootAddr, &pos);
	pos += SECTORS(bootSize);

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
	*ap++ = SECTORS(bootSize);		/* Boot sectors */
	*ap++ = bootAddr & 0xFF;			/* Boot addr [0..7] */
	*ap++ = (bootAddr >> 8) & 0xFF;		/* Boot addr [8..15] */
	*ap++ = (bootAddr >> 16) & 0xFF;	/* Boot addr [16..23] */

	/* The last two bytes is signature. */
	buf[SIGNATURE_POS] = SIGNATURE & 0xFF;
	buf[SIGNATURE_POS + 1] = (SIGNATURE >> 8) & 0xFF;


	/* Install images to device. */
	installImages(imgNames, pos, bootSize, &imgSize);

	/* Install params. */
	installParams(&buf[SECTOR_SIZE], pos, imgSize);

	/* Write bootBlock to device. */
	sDev(OFFSET(lowSector));
	wDev(buf, BOOT_BLOCK_SIZE);
}

void readBlock(Off_t blockNum, char *buf, int blockSize) {
/* For rawfs, so that it can read blocks. */
	sDev(OFFSET(lowSector) + blockNum * blockSize);
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
