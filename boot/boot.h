#ifndef _BOOT_H
#define _BOOT_H

#include "sys/types.h"

#define DEBUG			false
#define	SECTOR_SIZE		512

/**
 * etext  This is the first address past the end of the text segment (the program code).
 *
 * edata  This is the first address past the end of the initialized data segment.
 *
 * end    This is the first address past the end of the uninitialized data segment 
 *		  (also known as the BSS segment).
 *
 * The symbols must have some type, or "gcc -Wall" complains.
 */
extern char etext, edata, end;


extern void rawCopy(char *newAddr, char *oldAddr, u32_t size);
extern void relocate();

u32_t caddr;
u16_t runSize, device;

typedef struct {		// 8086 vector
	u16_t offset;
	u16_t segment;
} vector;

vector bootPartEntry;		// Boot partition table entry.

extern char* mon2Abs(void *pos);
extern char* vec2Abs(vector *vec);

typedef struct {		// One chunk of free memory.
	u32_t base;			// Start byte.
	u32_t size;			// Number of bytes.
} memory;

memory mem[3];			// List of available memory.

u32_t lowSector;

#endif
