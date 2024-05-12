#define _POSIX_SOURCE	1
#define _MINIX			1

#include <code.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/dir.h>
#include <limits.h>
#include <image.h>
#include <sys/stat.h>
#include "util.h"
#include "boot.h"
#include "rawfs.h"

#define BUFFERED_SECTORS	16
#define PROCESS_MAX			16	
#define KERNEL				0	/* The first process is the kernel. */	

#define align(p, n)		(((u32_t)(p) + ((u32_t)(n) - 1)) & ~((u32_t)(n) - 1))

typedef struct {
	u32_t	entry;		/* Entry point. */
	u32_t	cs;			/* Code segment. */
	u32_t	ds;			/* Data segment. */
} Process;
Process procs[PROCESS_MAX];

static off_t imgOff, imgSize;
static u32_t (*vir2Sec)(u32_t vsec);
static int blockSize = 0;

/* Simply add an absolute sector offset to vsec. */
static u32_t flatVir2Sec(u32_t vsec) {
	return lowSector + imgOff + vsec;
}

/* Translate a virtual sector number to an absolute disk sector. */
u32_t fileVir2Sec(u32_t vsec) {
	off_t block;
	int secsPerBlk;

	if (! blockSize) {
		errno = 0;
		return -1;
	}

	secsPerBlk = RATIO(blockSize);
	if ((block = rawVir2Abs(vsec / secsPerBlk)) == -1) {
		errno = EIO;
		return -1;
	}
	return block == 0 ? 
		0 : 
		lowSector + block * secsPerBlk + vsec % secsPerBlk;
}

static ino_t latestVersion(char *version, struct stat *stp) {
/* Recursively read the current directory, selecting the newest image on
 * the way up. (One can't use rawStat while reading a directory.)
 */
	char name[NAME_MAX + 1];
	ino_t ino, newest;
	time_t mtime;

	if ((ino = rawReadDir(name)) <= 0) {
		stp->st_mtime = 0;
		return 0;
	}

	newest = latestVersion(version, stp);
	mtime = stp->st_mtime;
	rawStat(ino, stp);

	if (S_ISREG(stp->st_mode) && stp->st_mtime > mtime) {
		newest = ino;
		strcpy(version, name);
	} else {
		stp->st_mtime = mtime;
	}
	return newest;
}

static char *selectImage(char *image) {
/* Look imag eup on the filesystem, if it is a file then we're done, but
 * if it's a directory then we want the newest file in that directory. If
 * it doesn't exist at all, thn see if it is 'number:number' and get the
 * image from that absolute offset off the disk.
 */
	size_t len;
	ino_t imageIno;
	struct stat st;
	
	len = (strlen(image) + 1 + NAME_MAX + 1) * sizeof(char);
	image = strcpy(malloc(len), image);

	fsOK = rawSuper(&blockSize) != 0;
	if (! fsOK || (imageIno = rawLookup(ROOT_INO, image)) == 0) {
		char *size;

		if (numPrefix(image, &size) && *size++ == ':' && numeric(size)) {
			vir2Sec = flatVir2Sec;
			imgOff = a2l(image);
			imgSize = a2l(size);
			strcpy(image, "Minix");
			return image;
		}
		if (! fsOK) 
		  printf("No image selected\n");
		else
		  printf("Can't load %s: %s\n", image, unixErr(errno));
		goto bailOut;
	}

	rawStat(imageIno, &st);
	if (! S_ISREG(st.st_mode)) {
		char *version = image + strlen(image);
		char dots[NAME_MAX + 1];

		if (! S_ISDIR(st.st_mode)) {
			printf("%s: %s\n", image, unixErr(ENOTDIR));
			goto bailOut;
		}

		rawReadDir(dots);
		rawReadDir(dots);	/* "." & ".." */
		*version++ = '/';
		*version = '\0';
		if ((imageIno = latestVersion(version, &st)) == 0) {
			printf("There are no images in %s\n", image);
			goto bailOut;
		}
		rawStat(imageIno, &st);
	}
	vir2Sec = fileVir2Sec;
	imgSize = (st.st_size + SECTOR_SIZE - 1) >> SECTOR_SHIFT;
	return image;
	
bailOut:
	free(image);
	return NULL;
}

static void printPrettyImage(char *image) {
	bool up;
	int c;
	while ((c = *image++) != 0) {
		if (c =='/' || c == '_')
		  c = ' ';
		else if (c == 'r' && between('0', *image, '9')) {
			printf(" revision ");
			continue;
		}
		if (!up && between('a', c, 'z'))
		  c = c - 'a' + 'A';
		else if (between('A', c, 'Z'))
		  up = 1;
		
		putch(c);
	}
}

/**
 * Try to read an entire track, so the next request is 
 * usually satisfied from the track buffer.
 */
static char *getSector(u32_t vsec) {
	u32_t sec;
	int r;
	static char buf[BUFFERED_SECTORS * SECTOR_SIZE];
	static size_t count;	/* Number of sectors in the buffer. */
	static u32_t bufSec;	/* First sector now in the buffer. */

	if (vsec == 0)
	  count = 0;			/* First sector, initialize. */

	if ((sec = vir2Sec(vsec)) == -1)
	  return NULL;

	if (sec == 0) {
		/* There is a hole. */
		count = 0;
		memset(buf, 0, SECTOR_SIZE);
		return buf;
	}

	if (sec - bufSec < count) {
		/* There are still sector(s) in the buffer. */
		return buf + ((size_t) (sec - bufSec) << SECTOR_SHIFT);
	}

	/* Not in the buffer. */
	count = 0;
	bufSec = sec;

	/* Try to read a whole track if possible. */
	while (++count < BUFFERED_SECTORS && 
				!isDevBoundary(bufSec + count)) {
		++vsec;
		if ((sec = vir2Sec(vsec)) == -1)
		  break;

		/* Consecutive? */
		if (sec != bufSec + count)
		  break;
	}

	if ((r = readSectors(mon2Abs(buf), bufSec, count)) != 0) {
		readDiskError(bufSec, r);
		count = 0;
		errno = 0;
		return NULL;
	}

	return buf;
}

/* Clear "count" bytes at absolute address "addr". */
static void rawClear(u32_t addr, u32_t count) {
	static char zeros[128];
	u32_t dst;
	u32_t zct;

	zct = sizeof(zeros);
	if (zct > count)
	  zct = count;
	rawCopy((char *) addr, mon2Abs(&zeros), zct);
	count -= zct;

	while (count > 0) {
		dst = addr + zct;
		if (zct > count)
		  zct = count;
		rawCopy((char *) dst, (char *) addr, zct);
		count -= zct;
		zct *= 2;
	}
}

static bool isBadImage(char *procName, Elf32_Ehdr *ehdrPtr) {
	if (ehdrPtr->e_ident[EI_MAG0] != ELFMAG0 ||
				ehdrPtr->e_ident[EI_MAG1] != ELFMAG1 ||
				ehdrPtr->e_ident[EI_MAG2] != ELFMAG2 ||
				ehdrPtr->e_ident[EI_MAG3] != ELFMAG3) {
		printf("%s is not an ELF file.\n", procName);
		return true;
	}
	
	if (ehdrPtr->e_ident[EI_CLASS] != ELFCLASS32) {
		printf("%s is not an 32-bit executable.\n", procName);
		return true;
	}

	if (ehdrPtr->e_ident[EI_DATA] != ELFDATA2LSB) {
		printf("%s is not little endian.\n", procName);
		return true;
	}

	if (ehdrPtr->e_ident[EI_VERSION] != EV_CURRENT) {
		printf("%s has an invalid version.\n", procName);
		return true;
	}

	if (ehdrPtr->e_type != ET_EXEC) {
		printf("%s is not an executable.\n", procName);
		return true;
	}
	return false;
}

static bool isSelected(char *name) {
	char *colon, *label;
	int cmp;

	if ((colon = strchr(name, ':')) == NULL)
	  return true;
	if ((label = getVarValue("label")) == NULL)
	  return true;

	*colon = 0;
	cmp = strcmp(label, name);
	*colon = ':';
	return cmp == 0;
}

static u32_t getProcSize(ImgHdr *imgHdr) {
	Exec *exec = &imgHdr->process;
	u32_t len = align(exec->codeHdr.p_filesz, SECTOR_SIZE) + 
				align(exec->dataHdr.p_filesz, SECTOR_SIZE);
	return len >> SECTOR_SHIFT;
	
}

static bool getSegment(u32_t *vsecPtr, long size, u32_t *addrPtr, u32_t limit) {
	char *buf;
	size_t cnt, n;
	u32_t vsec = *vsecPtr;
	u32_t addr = *addrPtr;

	cnt = 0;
	while (size > 0) {
		if (cnt == 0) {
			/* Get one buffered sector. */
			if ((buf = getSector(vsec++)) == NULL)
			  return false;
			cnt = SECTOR_SIZE;
		}

		/* Check if memory is enough. */
		if (addr + size > limit) {
			errno = ENOMEM;
			return false;
		}

		n = size;
		if (n > cnt)
		  n = cnt;
		/* Copy from buffer to addr */
		rawCopy((char *) addr, mon2Abs(buf), n);
		
		addr += n;
		size -= n;
		buf += n;
		cnt -= n;
	}
	*vsecPtr = vsec;
	*addrPtr = addr;
	return true;
}

static bool copyParams(char *params, size_t size) {
	size_t i = 0, n;
	Environment *e;
	char *name, *value;
	dev_t dev;

	memset(params, 0, size);

	for (e = env; e != NULL; e = e->next) {
		if (e->flags & E_VAR) {
			name = e->name;
			value = e->value;

			if (e->flags & E_DEV) {
				if ((dev = name2Dev(value)) == -1)
				  return 0;
				value = ul2a10((u16_t) dev);
			}
			/* Format: name=value\0 */
			n = i + strlen(name) + 1 + strlen(value) + 1;
			if (n < size) {
				strcpy(params + i, name);	
				strcat(params + i, "=");
				strcat(params + i, value);
			}
			i = n;
		}
	}

	if (i >= size) {
		printf("Too many boot parameters\n");
		return false;
	}
	params[i] = 0;	/* End marked with empty string. */
	return true;
}

static void printParams(char *params) {
	char *p = params;
	while (p != NULL && *p != '\0') {
		printf("%s\n", p);
		p = strchr(p, '\0');
		p++;
	}
}

static void execImage(char *image) {
	char *delayValue;
	char params[SECTOR_SIZE];
	ImgHdr imgHdr;
	Exec *exec;
	u32_t vsec, addr, limit, imgHdrPos;
	bool banner = false;
	Process *procp;
	size_t hdrLen, dataSize, bssSize, stackSize;
	int i;
	char *buf;
	char *console;
	u16_t mode;
	extern char *sbrk(int);
	u32_t dataAddr, dsClicks, offsetClicks, offsetBytes;

	sbrk(0);

	printf("\nLoading ");
	printPrettyImage(image);
	printf(".\n\n");

	vsec = 0;
	hdrLen = PROCESS_MAX * EXEC_SIZE;
	addr = memList[0].base;

	/* If code is relocated in memList[0], then limit must be <= caddr */
	limit = min(caddr, memList[0].base + memList[0].size);
	/* Left space for image headers */
	limit -= hdrLen;
	imgHdrPos = limit;

	/* Clear the area where the headers will be placed. */
	rawClear(imgHdrPos, hdrLen);
	for (i = 0; vsec < imgSize; ++i) {
		if (i == PROCESS_MAX) {
			printf("There are more than %d programs in %s\n", PROCESS_MAX, image);
			errno = 0;
			return;
		}
		procp = &procs[i];

		/* Read header. */
		while (true) {
			if ((buf = getSector(vsec++)) == NULL)
			  return;

			memcpy(&imgHdr, buf, sizeof(imgHdr));
			exec = &imgHdr.process;

			if (isBadImage(imgHdr.name, &exec->ehdr)) {
				errno = ENOEXEC;
				return;
			}

			/* Check the optional label on the process. */
			if (isSelected(imgHdr.name))
			  break;

			/* Bad label, skip this process */
			vsec += getProcSize(&imgHdr);
		}

	    addr = align(addr, CLICK_SIZE);

		if (!banner) {
			printf("     cs       ds      off     text     data      bss    stack\n");
			banner = true;
		}
		
		procp->cs = addr;
		procp->entry = exec->ehdr.e_entry;

		/* Save a copy of the header for the kernel, with the address
		 * where the process is loaded at.
		 */
		exec->codeHdr.p_paddr = addr;

		/* Read the text segment. */
		if (!getSegment(&vsec, exec->codeHdr.p_filesz, &addr, limit))
		  return;

		/* Read the data segment. */
		dataAddr = align(addr, CLICK_SIZE);
		dsClicks = dataAddr >> CLICK_SHIFT;
		offsetClicks = exec->dataHdr.p_vaddr >> CLICK_SHIFT;
		if (offsetClicks > dsClicks)
		  offsetClicks = dsClicks;
#ifdef _NO_COMPACT
		offsetClicks = 0;
#endif
		offsetBytes = offsetClicks << CLICK_SHIFT;
		procp->ds = dataAddr - offsetBytes;
		dataAddr += exec->dataHdr.p_vaddr - offsetBytes;
		rawClear(addr, dataAddr - addr);
		exec->dataHdr.p_paddr = procp->ds;
		exec->dataHdr.p_offset = offsetBytes;
		addr = dataAddr;

		dataSize = exec->dataHdr.p_filesz;
		bssSize = exec->dataHdr.p_memsz - dataSize;
		stackSize = exec->stackHdr.p_memsz;	

		/* Read the data segment. */
		if (!getSegment(&vsec, dataSize, &addr, limit)) 
		  return;

		if (addr + bssSize + stackSize > limit) {
			errno = ENOMEM;
			return;
		}
		/* Zero out bss. */
		rawClear(addr, bssSize + stackSize);
		addr += bssSize + stackSize;

		rawCopy((char *) (imgHdrPos + i * EXEC_SIZE), mon2Abs(exec), EXEC_SIZE);

		printf("%07lx  %07lx %8lx %8lx %8lx %8lx %8lx    %s\n",
			procp->cs, procp->ds, offsetBytes, exec->codeHdr.p_filesz, 
			dataSize, bssSize, stackSize, imgHdr.name);

		if (i == 0) {
			addr = memList[1].base;
			limit = memList[1].base + memList[1].size;
		}
	}

	if (i == 0) {
		printf("There are no programs in %s\n", image);
		errno = 0;
		return;
	}
	
	/* Do delay if wanted. */
	if ((delayValue = getVarValue("bootdelay")) != NULL)
	  delay(a2l(delayValue));

	/* Run the trailer function just before starting Minix. */
	if (!runTrailer()) {
		errno = 0;
		return;
	}

	/* Translate the boot parameters to what Minix likes best. */
	if (!copyParams(params, sizeof(params))) {
		errno = 0;
		return;
	}
	if (false) printParams(params);

	/* Set the video to the required mode.
	 * You can try to change the console or the chrome, and you
	 * will get a interesting display. See setVideoMode() for more.
	 */
	if ((console = getVarValue("console")) == NULL || 
				(mode = a2x(console)) == 0) {
		mode = strcmp(getVarValue("chrome"), "color") == 0 ? 
						COLOR_MODE : MONO_MODE;
	}
	setVideoMode(mode);

	/* Close the disk. */
	closeDev();

	/* Minix. */
	minix(procs[KERNEL].entry, procs[KERNEL].cs, procs[KERNEL].ds, 
				params, sizeof(params), imgHdrPos);

	//TODO copy rebootCode
	
	parseCode(params);

	/* Return from Minix. Things may have changed, so assume nothing. */
	fsOK = -1;
	errno = 0;

	/* Read leftover character, if any. */
	scanKeyboard();
}

void bootMinix() {
	char *imgName, *image;

	imgName = getVarValue("image");
	if ((image = selectImage(imgName)) == NULL)
	  return;

	execImage(image);

	switch (errno) {
		case ENOEXEC:
			printf("%s contains a bad program header\n", image);
			break;
		case ENOMEM:
			printf("Not enough memory to load %s\n", image);
			break;
		case EIO:
			printf("Unsuspected EOF on %s\n", image);
			break;
	}
	free(image);
}
