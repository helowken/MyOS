#ifndef _IMAGE_H
#define _IMAGE_H

#include "elf.h"

#define IMG_NAME_MAX	63
#define isRX(p)			(((p)->p_flags & PF_R) && ((p)->p_flags & PF_X))
#define isRW(p)			(((p)->p_flags & PF_R) && ((p)->p_flags & PF_W))
#define isPLoad(p)		((p)->p_type == PT_LOAD)

typedef struct {
	Elf32_Ehdr ehdr;	/* ELF header */
	Elf32_Phdr codeHdr;	/* Program header for text and rodata */
	Elf32_Phdr dataHdr;	/* Program header for data and bss */
} Exec;

typedef struct {
	char name[IMG_NAME_MAX + 1];	/* Null terminated. */
	Exec process;
} ImageHeader;

#define EXEC_SIZE		(sizeof(Exec))

#define IMG_STACK_SIZE	1024		/* TODO: Add stack to image's bss */
#define MEM_SIZE(hdr)	((hdr)->p_vaddr + (hdr)->p_memsz + IMG_STACK_SIZE)	/*TODO*/

#endif
