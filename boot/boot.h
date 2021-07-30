#include "sys/types.h"

#define STACK_SIZE 0x2800	// Assumed this code needs 10K stack

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

typedef struct {
	u32_t base;
	u32_t size;
} memory;

memory mem[3];
