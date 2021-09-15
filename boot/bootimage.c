#include "code.h"
#include "errno.h"
#include "stdlib.h"
#include "util.h"
#include "sys/dir.h"
#include "limits.h"
#include "unistd.h"
#include "boot.h"

#define SECTORS_IN_BUF	16

static off_t imageOffset, imageSize;
static u32_t (*vir2Sec)(u32_t vsec);		

/* Simply add an absolute sector offset to vsec. */
static u32_t flatVir2Sec(u32_t vsec) {
	return lowSector + imageOffset + vsec;
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
		imageOffset = a2l(image);
		imageSize = a2l(size);
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
	static char buf[SECTORS_IN_BUF * SECTOR_SIZE];
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
	while (++count < SECTORS_IN_BUF && 
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

static void execImage(char *image) {
	sbrk(0);
	
	printf("\nLoading ");
	printPrettyImage(image);
	printf(".\n\n");

	if (false) getSector(0);
}

void bootMinix() {
	char *imageName, *image;

	imageName = getVarValue("image");
	if ((image = selectImage(imageName)) == NULL)
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
