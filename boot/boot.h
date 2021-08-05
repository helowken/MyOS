#include "sys/types.h"

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

extern void rawCopy(u32_t newAddr, u32_t oldAddr, u32_t size);
extern void relocate();

bool debug = false;

u32_t caddr;
u16_t runSize, device;

typedef struct {		// 8086 vector
	u16_t offset;
	u16_t segment;
} vector;

vector rem_part;		// Boot partition table entry.

typedef struct {		// One chunk of free memory.
	u32_t base;			// Start byte.
	u32_t size;			// Number of bytes.
} memory;

memory mem[3];			// List of available memory.

