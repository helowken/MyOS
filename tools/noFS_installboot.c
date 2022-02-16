#include "common.h"
#include "util.h"
#include "image.h"

#define MASTER_BOOT_LEN		440
#define BOOT_BLOCK_LEN		512			/* Like standard UNIX, reserves 1K block of disk device
										   as a bootblock, but only one 512-byte sector is loaded
										   by the master boot sector, so 512 bytes are available
										   for saving settings (params) */
#define PARAM_LEN			512
#define SIGNATURE_POS		510
#define SIGNATURE			0XAA55
#define BOOT_MAX_SECTORS	0x80		/* bootable max size must be <= 64K, since BIOS has a 
										   DMA 64K boundary on loading data from disk. 
										   See biosDiskError(0x09) in boot.c. */
#define BOOT_SEC_OFF		8			/* bootable offset in device */

#define isRX(p)				(((p)->p_flags & PF_R) && ((p)->p_flags & PF_X))
#define isRW(p)				(((p)->p_flags & PF_R) && ((p)->p_flags & PF_W))
#define ALIGN(n)			(((n) + ((SECTOR_SIZE) - 1)) & ~((SECTOR_SIZE) - 1))

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
	  "       installboot -i(mage) image kernel mm fs ... init\n"
	  "       installboot -d(evice) device bootblock boot\n"
	  "       installboot -b(ootable) device bootblock\n");
	exit(1);
}

static uint32_t getLowSector(int deviceFd) {
	PartitionEntry pe;

	getActivePartition(deviceFd, &pe);
	return pe.lowSector;
}

typedef void (*preCopyFunc)(int destFd, int srcFd);

static void installMasterboot(char *device, char *masterboot) {
	int deviceFd, mbrFd;
	char buf[MASTER_BOOT_LEN];

	memset(buf, 0, MASTER_BOOT_LEN);
	deviceFd = WOpen(device);
	mbrFd = ROpen(masterboot);
	Read(masterboot, mbrFd, buf, MASTER_BOOT_LEN);
	Write(device, deviceFd, buf, MASTER_BOOT_LEN);
	Close(masterboot, mbrFd);
	Close(device, deviceFd);
}

static void installBootable(char *device, char *boot) {
	off_t size;
	int deviceFd, bootFd, sectors;
	uint32_t lba;

	size = getFileSize(boot);
	sectors = SECTORS(size);
	if (sectors > BOOT_MAX_SECTORS)
	  fatal("Bootable size > 64K.");

	int len = OFFSET(sectors);
	char buf[len];

	memset(buf, 0, len);
	bootFd = ROpen(boot);
	Read(boot, bootFd, buf, len);
	Close(boot, bootFd);

	deviceFd = RWOpen(device);
	lba = getLowSector(deviceFd);
	Lseek(device, deviceFd, OFFSET(BOOT_SEC_OFF + lba));
	Write(device, deviceFd, buf, len);
	Close(device, deviceFd);
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

static long totalText = 0, totalData = 0, totalBss = 0;

static void readHeader(char *procName, FILE *procFile, ImageHeader *imgHdr) {
	int i, sn = 0;
	static bool banner = false;
	Exec *proc = &imgHdr->process;
	Elf32_Ehdr *ehdrPtr = &proc->ehdr;
	Elf32_Phdr phdr;
	size_t phdrSize, textSize = 0, dataSize = 0, bssSize = 0;

	memset(imgHdr, 0, sizeof(*imgHdr));
	strncpy(imgHdr->name, procName, IMG_NAME_MAX);

	if (fread(ehdrPtr, sizeof(*ehdrPtr), 1, procFile) != 1 || 
				ferror(procFile))
	  errExit("fread elf header");

	checkElfHeader(procName, ehdrPtr);

	if (fseek(procFile, ehdrPtr->e_phoff, SEEK_SET) != 0)
	  errExit("fseek");

	phdrSize = ehdrPtr->e_phentsize;	
	for (i = 0; i < ehdrPtr->e_phnum; ++i) {
		if (fread(&phdr, phdrSize, 1, procFile) != 1 || 
					ferror(procFile))
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

static void copyExec(char *procName, FILE *procFile, Elf32_Phdr *hdr) {
	size_t size = hdr->p_filesz;
	int padLen;

	if (size == 0)
	  return;
	
	padLen = ALIGN(size) - size;

	adjustBuf(size);

	if (fseek(procFile, hdr->p_offset, SEEK_SET) == -1)
	  errExit("fseek %s", procName);
	if (fread(currBuf, size, 1, procFile) != 1 || ferror(procFile))
	  errExit("copyExec fread %s", procName);

	bufOff += size;

	padImage(padLen);
}

static void installParams(char *device, int deviceFd, uint32_t lba, char *other) {
	char buf[PARAM_LEN];

	memset(buf, ';', PARAM_LEN);
	if (snprintf(buf, PARAM_LEN, paramsTpl, device, device, other) < 0)
	  errExit("snprintf params");

	Lseek(device, deviceFd, OFFSET(lba + PARAM_SEC_OFF));
	Write(device, deviceFd, buf, PARAM_LEN);
}

static void installImage(char *device, char **procNames) {
	FILE *procFile;
#define IMG_STR_LEN	50
	char *procName, *file, imgStr[IMG_STR_LEN];
	struct stat st;
	imgBuf = malloc(bufLen);
	ImageHeader imgHdr;
	Exec *proc;
	uint32_t lba;
	off_t secOff;
	int deviceFd, n;

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

		/* Use on sector to store exec header */
		readHeader(procName, procFile, &imgHdr);
		bwrite(&imgHdr, sizeof(imgHdr));
		padImage(SECTOR_SIZE - sizeof(imgHdr));
		
		proc = &imgHdr.process;
		if (isPLoad(&proc->codeHdr))
		  copyExec(procName, procFile, &proc->codeHdr);
		if (isPLoad(&proc->dataHdr))
		  copyExec(procName, procFile, &proc->dataHdr);
		
		fclose(procFile);
	}
	printf("   ------   ------   ------   ------\n");
	printf(" %8ld %8ld %8ld %8ld	total\n", totalText, totalData, totalBss, 
				totalText + totalData + totalBss);

	deviceFd = RWOpen(device);
	lba = getLowSector(deviceFd);
	secOff = BOOT_SEC_OFF + BOOT_MAX_SECTORS;
	Lseek(device, deviceFd, OFFSET(lba + secOff));
	Write(device, deviceFd, imgBuf, bufOff);
	free(imgBuf);

	if ((n = snprintf(imgStr, IMG_STR_LEN, imgTpl, secOff, SECTORS(bufOff))) < 0)
	  fatal("create image string");
	imgStr[n] = '\0';
	installParams(device, deviceFd, lba, imgStr);

	Close(device, deviceFd);
}

static void installDevice(char *device, char *bootblock, char *boot) {
	int addr = BOOT_SEC_OFF;
	char buf[BOOT_BLOCK_LEN];
	int deviceFd, bootblockFd;
	ssize_t size;
	off_t bootSize;
	char *ap;
	uint32_t lba;

	memset(buf, 0, BOOT_BLOCK_LEN);

	bootblockFd = ROpen(bootblock);
	size = Read(bootblock, bootblockFd, buf, BOOT_BLOCK_LEN);
	Close(bootblock, bootblockFd);

	/* Append boot size and address. */
	bootSize = getFileSize(boot);
	ap = &buf[size];
	*ap++ = SECTORS(bootSize);
	*ap++ = addr & 0xFF;
	*ap++ = (addr >> 8) & 0xFF;
	*ap++ = (addr >> 16) & 0xFF;

	/* The last two bytes is signature. */
	buf[SIGNATURE_POS] = SIGNATURE & 0xFF;
	buf[SIGNATURE_POS + 1] = (SIGNATURE >> 8) & 0xFF;

	deviceFd = RWOpen(device);
	/* Get the first sector absolute address of the bootable partition. */
	lba = getLowSector(deviceFd);
	/* Move to LBA. */
	Lseek(device, deviceFd, OFFSET(lba));
	/* Write bootblock to device. */
	Write(device, deviceFd, buf, BOOT_BLOCK_LEN);

	/* Install params. */
	installParams(device, deviceFd, lba, "");

	Close(device, deviceFd);
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
	else if (isOpt(argv[1], "-image"))
	  installImage(argv[2], argv + 3);
	else if (argc >= 5 && isOpt(argv[1], "-device"))
	  installDevice(argv[2], argv[3], argv[4]);
	else if (isOpt(argv[1], "-bootable"))
	  installBootable(argv[2], argv[3]);
	else 
	  usage();

	exit(EXIT_SUCCESS);
}
