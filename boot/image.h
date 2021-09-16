#ifndef _IMAGE_H
#define _IMAGE_H

#include "elf.h"

#define IMAGE_NAME_MAX	63

typedef struct {
	Elf32_Ehdr	elfHeader;				/* ELF header */
	Elf32_Phdr	progHeaders[2];			/* Program headers, for text and data/bss */
} Exec;

typedef struct {
	char	name[IMAGE_NAME_MAX + 1];	/* Null terminated. */
	Exec	exec;						
} ImageHeader;

#endif
