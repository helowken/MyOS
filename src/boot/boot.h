#ifndef _BOOT_H
#define _BOOT_H

#include <sys/types.h>

#define	SECTOR_SIZE		512
#define RATIO(b)		((b) / SECTOR_SIZE)
#define	SECTOR_SHIFT	9			/* 2^9 = 512 */
#define PARAM_SECTOR	1			/* Sector containing boot parameters. */

#define ESC				'\33'		/* Escape key. */

#define MSEC_PER_TICK	55			/* Clock does 18.2 ticks per second. (1000 / 18.2 is about 55) */
#define TICKS_PER_DAY	0x1800B0L	/* After 24 hours it wraps (65543 * 24) */

/* BIOS video modes. */
#define MONO_MODE		0x07		/* 80x25 monochrome. */
#define	COLOR_MODE		0x03		/* 80x25 color. */

#define CLICK_SIZE		1024		/* Unit in which memory is allocated */
#define CLICK_SHIFT		10			/* Unit in which memory is allocated */

#ifndef EXTERN
#define EXTERN extern
#endif

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

extern char x_gdt[48];				/* Extend Memory Block Move. */
extern char p_gdt[64];				/* Global descriptor table */

EXTERN u32_t caddr;				/* Code address of the boot program. */
EXTERN u16_t runSize;			/* Size of this program. */
EXTERN u16_t device;			/* Drive being booted from. */

typedef struct {				/* 8086 vector */
	u16_t offset;
	u16_t segment;
} Vector;

EXTERN Vector bootPartEntry;	/* Boot partition table entry. */

typedef struct {				/* One chunk of free memory. */
	u32_t base;					/* Start byte. */
	u32_t size;					/* Number of bytes. */
} Memory;

EXTERN Memory memList[3];		/* List of available memory. */

extern int fsOK;				/* True if the boot device contains an FS. */
EXTERN u32_t lowSector;			/* Offset to the file system on the boot device. */

/* Sticky attributes. */
#define E_SPECIAL	0x01
#define E_DEV		0x02	
#define E_RESERVED	0x04	
#define E_STICKY	0x07	

/* Volatile attributes */
#define E_VAR		0x08		/* Valirable */
#define E_FUNCTION	0x10		/* Function definition. */

typedef struct Environment {
	struct Environment *next;
	char flags;
	char *name;		
	char *arg;
	char *value;
	char *defValue;
} Environment;

EXTERN Environment *env;		/* Lists the environment. */

/* Given a description text for some Unix errors. */
char *unixErr(int err);

/* Get value of env variable. */
char *getVarValue(char *name);

void readBlock(Off_t blockNum, char *buf, int bs);

void delay(u32_t msec);

/* Run the trailer function. */
bool runTrailer();

/* Load and start a Minix image. */
void bootMinix();

/* Parse boot monitor commands. */
void parseCode(char *code);

/* Report a read error. */
void readDiskError(off_t sector, int err);

/* Copy bytes from anywhere to anywhere. */
extern void rawCopy(char *newAddr, char *oldAddr, u32_t size);

/* Switch to a copy of this program. */
extern void relocate();

/* True if sector is on a track boundary. */
extern int isDevBoundary(u32_t sector);

/* Read 1 or more sectors from "device". */
extern int readSectors(char *buf, u32_t pos, int count);

/* Write 1 or more sectors to "device". */
extern int writeSectors(char *buf, u32_t pos, int count);

/* Exit the monitor. */
extern void exit(int status);

/* Translate a device name to a device number. */
dev_t name2Dev(char *name);

/* System bus type, XT, AT, or MCA. */
extern int getBus();

/* Display type, MDA to VGA. */
extern int getVideoMode();

extern void setVideoMode(unsigned mode);

/* Send a character to the screen. */
extern void putch(int ch);

/* Read a keypress. */
extern int getch();

/* Read keypress directly from keyboard controller. */
void scanKeyboard();

/* Undo a keypress. */
extern void ungetch();

/* True if escape typed. */
extern int escape();

/* Current value of the clock tick counter. */
extern u32_t getTick();

/* Local monitor address to absolute address. */
extern char *mon2Abs(void *pos);

/* Vector to absolute address. */
extern char *vec2Abs(Vector *vec);

/* Wait for an interrupt. */
extern void pause();

/* Close device. */
extern void closeDev();

/* Clear screen. */
extern void clearScreen();

/* Start Minix. */
extern void minix(u32_t kEntry, u32_t kcs, u32_t kds, 
			char *bootParams, size_t paramSize, u32_t headerPos);

#endif
