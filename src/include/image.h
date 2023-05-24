#ifndef _IMAGE_H
#define _IMAGE_H

#include <elf.h>

#define IMG_NAME_MAX	63
#define isRX(p)			(((p)->p_flags & PF_R) && ((p)->p_flags & PF_X))
#define isRW(p)			(((p)->p_flags & PF_R) && ((p)->p_flags & PF_W))
#define isPLoad(p)		((p)->p_type == PT_LOAD)
#define isStack(p)		((p)->p_type == PT_GNU_STACK)

typedef struct {
	Elf32_Ehdr ehdr;		/* ELF header */
	Elf32_Phdr codeHdr;		/* Program header for text and rodata */
	Elf32_Phdr dataHdr;		/* Program header for data and bss */
	Elf32_Phdr stackHdr;	/* Program header for data and bss */
} Exec;

typedef struct {
	char name[IMG_NAME_MAX + 1];	/* Null terminated. */
	Exec process;
} ImgHdr;

#define EXEC_SIZE		(sizeof(Exec))

#endif
