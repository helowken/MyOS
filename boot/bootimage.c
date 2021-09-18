#include "code.h"
#include "errno.h"
#include "stdlib.h"
#include "util.h"
#include "sys/dir.h"
#include "limits.h"
#include "unistd.h"
#include "image.h"
#include "boot.h"

#define BUF_SECTORS	16
#define PROCESS_MAX	16	
#define KERNEL		0	/* The first process is the kernel. */	

typedef struct {
	u32_t	entry;		/* Entry point. */
	u32_t	cs;			/* Code segment. */
	u32_t	ds;			/* Data segment. */
} Process;
Process procs[PROCESS_MAX];

static off_t imgOff, imgSize;
static u32_t (*vir2Sec)(u32_t vsec);		

/* Simply add an absolute sector offset to vsec. */
static u32_t flatVir2Sec(u32_t vsec) {
	return lowSector + imgOff + vsec;
}

static char *selectImage(char *image) {
	size_t len;
	char *size;
	
	len = (strlen(image) + 1 + NAME_MAX + 1) * sizeof(char);
	image = strcpy(malloc(len), image);

	/* TODO lookup from file system */
	
	if (numPrefix(image, &size) && 
				*size++ == ':' &&
				numeric(size)) {
		vir2Sec = flatVir2Sec;
		imgOff = a2l(image);
		imgSize = a2l(size);
		strcpy(image, "Minix/3.0.1r10");
		return image;
	}

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
	static char buf[BUF_SECTORS * SECTOR_SIZE];
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
	while (++count < BUF_SECTORS && 
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
	u32_t dst, zct;

	zct = sizeof(zeros);
	if (zct > count)
	  zct = count;
	rawCopy((char*) addr, mon2Abs(&zeros), zct);
	count -= zct;

	while (count > 0) {
		dst = addr + zct;
		if (zct > count)
		  zct = count;
		rawCopy((char *) dst, mon2Abs(&zeros), zct);
		count -= zct;
		zct *= 2;
	}
}

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

static void execImage(char *image) {
	ImageHeader imgHdr;
	u32_t vsec, addr, limit, imgHdrPos;
	//Process *procp;
	size_t hdrLen;
	int i;
	char *buf;

	sbrk(0);
	
	printf("\nLoading ");
	printPrettyImage(image);
	printf(".\n\n");

	vsec = 0;
	addr = memList[0].base;
	limit = addr + memList[0].size;
	if (limit > caddr)
	  limit = caddr;

	hdrLen = PROCESS_MAX * sizeof(imgHdr);
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
		//procp = &procs[i];

		while (true) {
			if ((buf = getSector(vsec++)) == NULL)
			  return;

			memcpy(&imgHdr, buf, sizeof(imgHdr));
			printProgramHeader(&imgHdr.codeHdr);
			printf("----------\n");
			printProgramHeader(&imgHdr.dataHdr);
			printf("=============\n");
			break;
		}
	}
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
