#include "common.h"
#include "dirent.h"
#include "../boot/rawfs.h"

#define MASTER_BOOT_SIZE	440
#define BOOT_BLOCK_OFF		0			/* Boot block offset in partition */
#define BOOT_BLOCK_SIZE		OFFSET(2)	/* Like standard UNIX, reserves 1K block of disk device
										   as a bootBlock, but only one 512-byte sector is loaded
										   by the master boot sector, so 512 bytes are available
										   for saving settings (params) */
#define PARAM_SIZE			SECTOR_SIZE
#define SIGNATURE_POS		510
#define SIGNATURE			0xAA55
#define BOOT_MAX			64			/* bootable max size must be <= 64K, since BIOS has a 
										   DMA 64K boundary on loading data from disk. 
										   See biosDiskError(0x09) in boot.c. */
#define STACK_SIZE			0x1400		/* Stack and heap size. (since malloc() is allocate memory 
										   on stack when booting).
										   It should be aligned to the STACK_SIZE in boothead.s. */
#define DEVICE				"c0d0p0s0"
#define ALIGN(n, m)			(((n) + ((m) - 1)) & ~((m) - 1))
#define ALIGN_SECTOR(n)		ALIGN(n, SECTOR_SIZE)	
#define CLICK_SIZE			1024		/* Unit in which memory is allocated */
#define STACK_DIR			"stack"		/* Directory contains stack size files */

#define sDev(off)		Lseek(device, deviceFd, (off))
#define sPart(off)		sDev(OFFSET(lowSector) + (off))
#define rDev(buf,size)	Read(device, deviceFd, (buf), (size))
#define wDev(buf,size)	Write(device, deviceFd, (buf), (size)) 
#define cpDev(fileName, srcFd)	CopyTo(fileName, srcFd, deviceFd)

#define MODULE_NAME_LEN		30

typedef enum { BOOT, FS } HowTo;

static char *progName;
static char *device;
static int deviceFd;
static uint32_t bootSector, lowSector;
static char zero[SECTOR_SIZE];

static char *paramsTpl = 
	"rootdev=%s;"
	"ramimagedev=%s;"
	"ramsize=2000;"		/* For release.sh */
	"%s"
	"minix(1,Start MINIX 3 (requires at least 16 MB RAM)) { "
		//"unset image; "
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
	  "Usage: installboot -i(mage) image kernel pm fs ... init\n"
	  "       installboot -m(aster) device masterboot\n"
	  "       installboot -d(evice) device bootBlock boot [images ...]\n");
	exit(1);
}

static void installMasterboot(char *masterboot) {
/* Install masterboot to the first sector of device */
	int mbrFd;
	char buf[MASTER_BOOT_SIZE];

	memset(buf, 0, MASTER_BOOT_SIZE);
	mbrFd = ROpen(masterboot);
	Read(masterboot, mbrFd, buf, MASTER_BOOT_SIZE);
    sDev(OFFSET(bootSector));
	wDev(buf, MASTER_BOOT_SIZE);
	Close(masterboot, mbrFd);
}

static char *formatName(char *name) {
	char *np;

	if ((np = strrchr(name, '/')) != NULL)
	  name = np + 1;
	if ((np = strrchr(name, '.')) != NULL)
	  *np = 0;
	return name;
}

static long totalText = 0, totalData = 0, totalBss = 0, totalStack = 0;

static size_t readHeader(char *fileName, FILE *imgFile, ImgHdr *imgHdr, bool print) {
	int i;
	static bool banner = false;
	Exec *proc = &imgHdr->process;
	Elf32_Ehdr *ehdrPtr = &proc->ehdr;
	Elf32_Phdr phdr;
	size_t phdrSize, textSize = 0, dataSize = 0, bssSize = 0, stackSize = 0;
	size_t sizeInMemory = 0;
	bool textFound = false, dataFound = false, stackFound = false;

	memset(imgHdr, 0, sizeof(*imgHdr));
	strncpy(imgHdr->name, formatName(fileName), IMG_NAME_MAX);

	Fread(fileName, imgFile, ehdrPtr, sizeof(*ehdrPtr));
	checkElfHeader(fileName, ehdrPtr);

	Fseek(fileName, imgFile, ehdrPtr->e_phoff);
	phdrSize = ehdrPtr->e_phentsize;	
	for (i = 0; i < ehdrPtr->e_phnum; ++i) {
		Fread(fileName, imgFile, &phdr, phdrSize);

		if (isPLoad(&phdr)) {
			if (isRX(&phdr)) {
				if (textFound)
				  fatal("More than 1 text program header: %s", fileName);
				memcpy(&proc->codeHdr, &phdr, phdrSize);
				textSize = phdr.p_filesz;
				sizeInMemory = phdr.p_vaddr + phdr.p_memsz;
				textFound = true;
			} else if (isRW(&phdr)) {
				if (dataFound)
				  fatal("More than 1 data program header: %s", fileName);
				memcpy(&proc->dataHdr, &phdr, phdrSize);
				dataSize = phdr.p_filesz;
				bssSize = phdr.p_memsz - dataSize;
				sizeInMemory = phdr.p_vaddr + phdr.p_memsz;
				dataFound = true;
			} 	
		} else if (isStack(&phdr)) {
			if (stackFound)
			  fatal("More than 1 stack program header: %s", fileName);
			stackSize = phdr.p_memsz;
			proc->stackHdr.p_memsz = stackSize;
			stackFound = true;
		}
	}
	if (!textFound)
	  fatal("No text program header found: %s", fileName);
	if (!dataFound)
	  fatal("No data program header found: %s", fileName);
	if (!stackFound)
	  fatal("No stack program header found: %s", fileName);

	if (print) {
		if (!banner) {
			printf("\n     text     data      bss    stack     size\n");
			banner = true;
		}
		printf(" %8d %8d %8d %8d %8d	%s\n", textSize, dataSize, bssSize, 
					stackSize, textSize + dataSize + bssSize, fileName);

		totalText += textSize;
		totalData += dataSize;
		totalBss += bssSize;
		totalStack += stackSize;
	}
	
	return sizeInMemory;
}

static off_t imageTotalSize = 0;

static void bwrite(char *dstName, int dstFd, void *buf, size_t len) {
	imageTotalSize += len;
	Write(dstName, dstFd, buf, len);
}

static void padImage(char *dstName, int dstFd, size_t len) {
	imageTotalSize += len;
	Write(dstName, dstFd, zero, len);
}

static void copyExec(char *progName, FILE *imgFile, Elf32_Phdr *hdr,
					char *dstName, int dstFd) {
	size_t size = hdr->p_filesz;
	int padLen;
	char buf[BUFSIZ];
	int chunk;

	if (size == 0)
	  return;
	
	padLen = ALIGN_SECTOR(size) - size;
	imageTotalSize += size;

	Fseek(progName, imgFile, hdr->p_offset);
	
	while (size) {
		chunk = size;
		if (chunk > BUFSIZ)
		  chunk = BUFSIZ;
		Fread(progName, imgFile, buf, chunk);
		Write(dstName, dstFd, buf, chunk);
		size -= chunk;
	}
	padImage(dstName, dstFd, padLen);
}

static void doInstallImgs(char *dstName, int dstFd, char **imgNames) {
	FILE *imgFile;
	char *imgName, *file;
	ImgHdr imgHdr;
	Exec *proc;
	Elf32_Phdr *hdr;

	while ((imgName = *imgNames++)) {
		if ((file = strrchr(imgName, ':')) != NULL)
		  ++file;
		else
		  file = imgName;

		imgFile = RFopen(file);
		/* Use on sector to store exec header */
		readHeader(imgName, imgFile, &imgHdr, true);
		bwrite(dstName, dstFd, &imgHdr, sizeof(imgHdr));
		padImage(dstName, dstFd, SECTOR_SIZE - sizeof(imgHdr));

		proc = &imgHdr.process;
		/* text */
		hdr = &proc->codeHdr;
		copyExec(imgName, imgFile, hdr, dstName, dstFd);
		/* data */
		hdr = &proc->dataHdr;
		copyExec(imgName, imgFile, hdr, dstName, dstFd);
		/* stack */
		hdr = &proc->stackHdr;

		Fclose(file, imgFile);
	}
	printf("   ------   ------   ------   ------   ------\n");
	printf(" %8ld %8ld %8ld %8ld %8ld	total\n\n", totalText, totalData, totalBss, totalStack,
				totalText + totalData + totalBss);
}

static void installImages(char **imgNames, int imgAddr) {
/* Linux path */
	sPart(OFFSET(imgAddr));
	doInstallImgs(device, deviceFd, imgNames);
}

static void makeImage(char *image, char **imgNames) {
/* Minix Path */
	int fd;

	fd = Open(image, O_CREAT | O_WRONLY | O_TRUNC);
	doInstallImgs(image, fd, imgNames);
	Close(image, fd);
}

static void checkBootValid(char *boot) {
	FILE *bootFile;
	ImgHdr imgHdr;
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

	bootFile = RFopen(bootElf);
	size = readHeader(bootElf, bootFile, &imgHdr, false);
	if (size + STACK_SIZE > (BOOT_MAX << KB_SHIFT))
	  fatal("Boot size > %d KB.", BOOT_MAX);
}

static void installBoot(char *boot, off_t *bootSizePtr, off_t *bootAddrPtr, 
						int *secsPerBlkPtr) {
	int bootFd;
	Off_t fsSize;		/* Total blocks of the filesystem */
	int blockSize = 0;
	off_t bootSize;
	int bootAddr;
	Ino_t ino;

	/* Read and check the superblock. */
	fsSize = rawSuper(&blockSize);
	if (fsSize == 0)
	  fatal("%s: %s is not a Minix file system\n", progName, device);
	*secsPerBlkPtr = RATIO(blockSize);

	/* See if boot is present in the file system. */
	if ((ino = rawLookup(ROOT_INO, boot)) == 0) {
		if (errno != ENOENT)
		  fatal(boot);
	}
	if (ino == 0) {		/* Linux path */
		/* For a raw installation, we need to copy the boot code onto
		 * the device, so we need to look at the file to be copied.
		 */

		/* Calculate boot size and addr. */
		checkBootValid(boot);
		bootSize = getFileSize(boot);

		/* The boot will be appended immediately after FS. */
		bootAddr = fsSize * (*secsPerBlkPtr);

		bootFd = ROpen(boot);
		sPart(OFFSET(bootAddr));
		cpDev(boot, bootFd);
	} else {		/* Minix path */
		/* Boot is present in the file system. */
		struct stat st;

		rawStat(ino, &st);
		bootSize = st.st_size;
		
		/* Get the header from the first block. */
		if ((bootAddr = rawVir2Abs((off_t) 0)) == 0) {
			fatal("boot addr is invalid: %d", bootAddr);
		}
	}
	*bootSizePtr = bootSize;
	*bootAddrPtr = bootAddr;
}

static void installParams(char *params, char *imgStr) {
	memset(params, ';', PARAM_SIZE);
	if (snprintf(params, PARAM_SIZE, paramsTpl, DEVICE, DEVICE, imgStr) < 0)
	  errExit("snprintf params");
}

static void makeBootable(HowTo how, char *bootBlock, char *boot, char **imgNames) {
/* Install bootBlock to the boot sector with boot's disk addresses and sizes patched 
 * into the data segment of bootBlock. 
 */
	char buf[BOOT_BLOCK_SIZE];
	int bootBlockFd;
	ssize_t bootBlockSize;
	char *ap;		
	off_t bootAddr, bootSize, imgAddr, maxSize;
	int bootSectors, imgSectors, secsPerBlk;
	struct FileAddr {
		off_t address;
		int count;
	} bootAddrList[BOOT_MAX + 1], *bap = bootAddrList;
#define IMG_STR_LEN	30
	char imgStr[IMG_STR_LEN];
	int n;

	/* Read bootBlock to buf. */
	memset(buf, 0, BOOT_BLOCK_SIZE);
	bootBlockFd = ROpen(bootBlock);
	bootBlockSize = Read(bootBlock, bootBlockFd, buf, BOOT_BLOCK_SIZE);
	Close(bootBlock, bootBlockFd);

	/* Install boot to device. */
	installBoot(boot, &bootSize, &bootAddr, &secsPerBlk);
	bootSectors = SECTORS(bootSize);

	if (how == BOOT) {		/* Linux path */
		bap->address = bootAddr;
		bap->count = bootSectors;
	} else {		/* Minix path */
		off_t sector, addr;

		/* Determine the addresses to the boot to be patched into the 
		 * boot block.
		 */
		bap->count = 0;	/* Trick to get the address recording going. */

		for (sector = 0; sector < bootSectors; ++sector) {
			if ((addr = rawVir2Abs(sector / secsPerBlk)) == 0) 
			  fatal("%s: %s has holes! sector: %lu, addr: %lu\n", 
						  progName, boot, sector, addr);
			
			addr = (addr * secsPerBlk) + (sector % secsPerBlk);

			/* First address of the addresses array? */
			if (bap->count == 0)
			  bap->address = addr;

			/* Paste sectors together in a multisector read. */
			if (bap->address + bap->count == addr) {
				bap->count++;
			} else {
				/* New address. */
				bap++;
				bap->address = addr;
				bap->count = 1;
			}
		}
	}
	(++bap)->count = 0;		/* No more. */
	
	/* 4 = 1 size byte + 3 address bytes,
	 * the last 1 byte is the zero count to stop bootBlock's reading loop. */
	if (bootBlockSize + 4 * (bap - bootAddrList) + 1 >= SIGNATURE_POS) {
		fprintf(stderr, 
			"%s: %s + addresses to %s don't fit in the boot sector.", 
			progName, bootBlock, boot);
		fprintf(stderr,
			"You can try copying/reinstalling %s to defragment it\n",
			boot);
		exit(1);
	}

	/* Patch boot's size and address into the data segment of bootBlock. 
	 * (See "boot/bootBlock.asm".)
	 */
	ap = &buf[bootBlockSize];
	for (bap = bootAddrList; bap->count != 0; ++bap) {
		*ap++ = bap->count;		/* Boot sectors */
		*ap++ = (bap->address >> 0) & 0xFF;		/* Boot addr [0..7] */
		*ap++ = (bap->address >> 8) & 0xFF;		/* Boot addr [8..15] */
		*ap++ = (bap->address >> 16) & 0xFF;	/* Boot addr [16..23] */
	}
	/* Zero count stops bootBlock's reading loop. */
	*ap = 0;	

	/* The last two bytes is signature. */
	buf[SIGNATURE_POS] = SIGNATURE & 0xFF;
	buf[SIGNATURE_POS + 1] = (SIGNATURE >> 8) & 0xFF;

	if (how == BOOT) {		/* Linux path */
		/* Install images to device. */
		imgAddr = bootAddr + SECTORS(bootSize);
		installImages(imgNames, imgAddr);

		maxSize = OFFSET(RESERVED_SECTORS);
		if (bootSize + imageTotalSize > maxSize)
		  fatal("Total size of (boot + images) excceeds %dMB", 
				(maxSize >> KB_SHIFT) >> KB_SHIFT);

		imgSectors = SECTORS(imageTotalSize);
		n = snprintf(imgStr, IMG_STR_LEN, imgTpl, imgAddr, imgSectors);
#ifdef OTHER_OS
		printf("Image addr: %lu, sectors: %d\n", imgAddr, imgSectors);
#endif
	} else {
		imgAddr = 0;
		n = 0;
	}

	/* Install params. */
	imgStr[n] = 0;
	installParams(&buf[SECTOR_SIZE], imgStr);

	/* Write bootBlock to device. */
	sPart(BOOT_BLOCK_OFF);
	wDev(buf, BOOT_BLOCK_SIZE);

#ifdef OTHER_OS
	printf(" Boot addr: %lu, sectors: %d\n\n", bootAddr, bootSectors);
#endif
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

static void devInit(char *devStr) {
	PartitionEntry pe;

	device = parseDevice(devStr, &bootSector, &pe);
	lowSector = pe.lowSector;
	deviceFd = RWOpen(device);
}

int main(int argc, char *argv[]) {
	if (argc < 4 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	  usage();

	if ((progName = strrchr(argv[0], '/')) == NULL)
	  progName = argv[0];
	else
	  progName++;

	if (argc >= 4 && isOpt(argv[1], "-image")) {
		makeImage(argv[2], argv + 3);
	} else if (argc >= 4 && isOpt(argv[1], "-master")) {
		devInit(argv[2]);
	    installMasterboot(argv[3]);
	} else if (argc >= 5 && isOpt(argv[1], "-device")) {
		devInit(argv[2]);
	    makeBootable(FS, argv[3], argv[4], argv + 5); 
	} else if (argc >= 6 && isOpt(argv[1], "-boot")) {
		devInit(argv[2]);
	    makeBootable(BOOT, argv[3], argv[4], argv + 5); 
	} else {
		usage();
	}

	Close(device, deviceFd);

	return 0;
}
