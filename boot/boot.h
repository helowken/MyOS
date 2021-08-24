#ifndef _BOOT_H
#define _BOOT_H

#include "sys/types.h"

#define	SECTOR_SIZE		512

#define PARAM_SECTOR	1		// Sector containing boot parameters.

#define ESC		'\33'	// Escape key.

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
extern char x_gdt[48];

extern void rawCopy(char *newAddr, char *oldAddr, u32_t size);
extern void relocate();
extern int readSectors(char *buf, u32_t pos, int count);
extern void exit(int status);
extern int getBus();
extern int getVideoMode();
extern void putch(int ch);
extern int getch();
extern void ungetch();
extern int escape();

u32_t caddr;
u16_t runSize, device;

typedef struct {		// 8086 vector
	u16_t offset;
	u16_t segment;
} Vector;

Vector bootPartEntry;		// Boot partition table entry.

extern char* mon2Abs(void *pos);
extern char* vec2Abs(Vector *vec);

typedef struct {		// One chunk of free memory.
	u32_t base;			// Start byte.
	u32_t size;			// Number of bytes.
} Memory;

Memory memList[3];			// List of available memory.

u32_t lowSector;

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

Environment *env;		// Lists the environment.

#endif
