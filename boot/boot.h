#ifndef _BOOT_H
#define _BOOT_H

#include "sys/types.h"

#define	SECTOR_SIZE		512
#define PARAM_SECTOR	1			// Sector containing boot parameters.
#define ESC				'\33'		// Escape key.
#define MSEC_PER_TICK	55			// Clock does 18.2 ticks per second. (1000 / 18.2 is about 55)
#define TICKS_PER_DAY	0x1800B0L	// After 24 hours it wraps (65543 * 24)

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

extern char x_gdt[48];		// Extend Memory Block Move

u32_t caddr;				// Code address of the boot program.
u16_t runSize;				// Size of this program.
u16_t device;				// Drive being booted from.

typedef struct {			// 8086 vector
	u16_t offset;
	u16_t segment;
} Vector;

Vector bootPartEntry;		// Boot partition table entry.

typedef struct {			// One chunk of free memory.
	u32_t base;				// Start byte.
	u32_t size;				// Number of bytes.
} Memory;

Memory memList[3];			// List of available memory.

u32_t lowSector;			// Offset to the file system on the boot device.

// Sticky attributes.
#define E_SPECIAL	0x01
#define E_DEV		0x02	
#define E_RESERVED	0x04	
#define E_STICKY	0x08	

// Volatile attributes
#define E_VAR		0x08	// Valirable
#define E_FUNCTION	0x10	// Function definition.

typedef struct Environment {
	struct Environment *next;
	char flags;
	char *name;		
	char *arg;
	char *value;
	char *defValue;
} Environment;

Environment *env = NULL;	// Lists the environment.


// Copy bytes from anywhere to anywhere.
extern void rawCopy(char *newAddr, char *oldAddr, u32_t size);

// Switch to a copy of this program.
extern void relocate();

// Read 1 or more sectors from "device".
extern int readSectors(char *buf, u32_t pos, int count);

// Exit the monitor.
extern void exit(int status);

// System bus type, XT, AT, or MCA.
extern int getBus();

// Display type, MDA to VGA.
extern int getVideoMode();

// Send a character to the screen.
extern void putch(int ch);

// Read a keypress.
extern int getch();

// Undo a keypress.
extern void ungetch();

// True if escape typed.
extern int escape();

// Current value of the clock tick counter.
extern u32_t getTick();

// Local monitor address to absolute address.
extern char* mon2Abs(void *pos);

// Vector to absolute address.
extern char* vec2Abs(Vector *vec);

#endif
