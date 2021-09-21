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

#define align(p, n)		(((u32_t)(p) + ((u32_t)(n) - 1)) & ~((u32_t)(n) - 1))

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

	if ((sec = (*vir2Sec)(vsec)) == -1)
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
		if ((sec = (*vir2Sec)(vsec)) == -1)
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


static bool isBadImage(Elf32_Ehdr *ehdrPtr) {
	return (ehdrPtr->e_ident[EI_MAG0] != ELFMAG0 ||
				ehdrPtr->e_ident[EI_MAG1] != ELFMAG1 ||
				ehdrPtr->e_ident[EI_MAG2] != ELFMAG2 ||
				ehdrPtr->e_ident[EI_MAG3] != ELFMAG3) || 
		ehdrPtr->e_ident[EI_CLASS] != ELFCLASS32 ||
		ehdrPtr->e_ident[EI_DATA] != ELFDATA2LSB ||
		ehdrPtr->e_ident[EI_VERSION] != EV_CURRENT ||
		ehdrPtr->e_type != ET_EXEC;
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

static u32_t getProcSize(ImageHeader *imgHdr) {
	Exec *proc = &imgHdr->process;
	u32_t len = align(proc->codeHdr.p_filesz, SECTOR_SIZE) + 
				align(proc->dataHdr.p_filesz, SECTOR_SIZE);
	return len >> SECTOR_SHIFT;
	
}
static void checkElfHeader(char *procName, Elf32_Ehdr *ehdrPtr) {
	if (ehdrPtr->e_ident[EI_MAG0] != ELFMAG0 ||
				ehdrPtr->e_ident[EI_MAG1] != ELFMAG1 ||
				ehdrPtr->e_ident[EI_MAG2] != ELFMAG2 ||
				ehdrPtr->e_ident[EI_MAG3] != ELFMAG3)
	  printf("%s is not an ELF file.\n", procName);
	
	if (ehdrPtr->e_ident[EI_CLASS] != ELFCLASS32)
	  printf("%s is not an 32-bit executable.\n", procName);

	if (ehdrPtr->e_ident[EI_DATA] != ELFDATA2LSB)
	  printf("%s is not little endian.\n", procName);

	if (ehdrPtr->e_ident[EI_VERSION] != EV_CURRENT)
	  printf("%s has an invalid version.\n", procName);

	if (ehdrPtr->e_type != ET_EXEC)
	  printf("%s is not an executable.\n", procName);
}

static bool getSegment(u32_t *vsec, long size, u32_t *addr, u32_t limit) {
	char *buf;
	size_t cnt, n;

	cnt = 0;
	while (size > 0) {
		if (cnt == 0) {
			/* Get one buffered sector. */
			if ((buf = getSector((*vsec)++)) == NULL)
			  return false;
			cnt = SECTOR_SIZE;
		}

		/* Check if memory is enough. */
		if (*addr + size > limit) {
			errno = ENOMEM;
			return false;
		}

		n = size;
		if (n > cnt)
		  n = cnt;
		/* Copy from buffer to addr */
		rawCopy((char *) *addr, mon2Abs(buf), n);
		
		*addr += n;
		size -= n;
		buf += n;
		cnt -= n;
	}
	return true;
}

static void execImage(char *image) {
	ImageHeader imgHdr;
	Exec *proc;
	u32_t vsec, addr, limit, imgHdrPos;
	bool banner = false;
	Process *procp;
	size_t hdrLen, phdrLen, n, dataSize, bssSize;
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

	phdrLen = sizeof(*proc);
	hdrLen = PROCESS_MAX * phdrLen;
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
			proc = &imgHdr.process;

			checkElfHeader(imgHdr.name, &proc->ehdr);

			if (isBadImage(&proc->ehdr)) {
				errno = ENOEXEC;
				return;
			}

			/* Check the optional label on the process. */
			if (isSelected(imgHdr.name))
			  break;

			/* Bad label, skip this process */
			vsec += getProcSize(&imgHdr);
		}
		if (true) {
		printProgramHeader(&proc->codeHdr);
		printf("----------------\n");
		printProgramHeader(&proc->dataHdr);
		printf("================\n");
		}

		//if (i == KERNEL) 
	    addr = align(addr, proc->codeHdr.p_align);

		rawCopy((char *) (imgHdrPos + i * phdrLen), mon2Abs(proc), phdrLen);

		if (!banner) {
			printf("     cs       ds     text     data      bss\n");
			banner = true;
		}
		
		procp->cs = procp->ds = addr;
		procp->entry = proc->ehdr.e_entry;

		if (true) continue;

		/* Read the text segment. */
		if (!getSegment(&vsec, proc->codeHdr.p_filesz, &addr, limit))
		  return;

		dataSize = bssSize = 0;
		if (isPLoad(&proc->dataHdr)) {
			n = align(addr, proc->dataHdr.p_align) - addr;
			rawClear(addr, n);
			addr += n;
			dataSize = proc->dataHdr.p_filesz;
			bssSize = proc->dataHdr.p_memsz - dataSize;

			/* Read the data segment. */
			if (!getSegment(&vsec, proc->dataHdr.p_filesz, &addr, limit))
			  return;

			if (addr + bssSize > limit) {
				errno = ENOMEM;
				return;
			}
			/* Zero out bss. */
			rawClear(addr, bssSize);
			addr += bssSize;
		}

		printf("%07lx  %07lx %8ld %8ld %8ld  %s\n",
			procp->cs, procp->ds, proc->codeHdr.p_filesz, 
			dataSize, bssSize, imgHdr.name);

		if (i == 0) {
			addr = memList[1].base;
			limit = memList[1].base + memList[1].size;
		}

		if (i == 1)
		  break;
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
