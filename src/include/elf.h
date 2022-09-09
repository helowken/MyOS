#ifndef _ELF_H
#define _ELF_H

#include "stdint.h"

/* Type for a 16-bit quantity. */
typedef uint16_t Elf32_Half;

/* Types for signed and unsigned 32-bit quantities. */
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;		

/* Type of addresses. */
typedef uint32_t Elf32_Addr;

/* Type of file offsets. */
typedef uint32_t Elf32_Off;

/* Type of section indices, which are 16-bit quantities. */
typedef uint16_t Elf32_Section;

#define EI_NIDENT	(16)

typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
	Elf32_Half	e_type;			/* Object file type */
	Elf32_Half	e_machine;		/* Architecture */
	Elf32_Word	e_version;		/* Object file version */
	Elf32_Addr	e_entry;		/* Entry point virtual address */
	Elf32_Off	e_phoff;		/* Program header table file offset */
	Elf32_Off	e_shoff;		/* Section header table file offset */
	Elf32_Word	e_flags;		/* Processor-specific flags */
	Elf32_Half	e_ehsize;		/* ELF header size in bytes */
	Elf32_Half	e_phentsize;	/* Program header table entry size */
	Elf32_Half	e_phnum;		/* Program header table entry count */
	Elf32_Half	e_shentsize;	/* Section header table entry size */
	Elf32_Half	e_shnum;		/* Section header table entry count */
	Elf32_Half	e_shstrndx;		/* Section header string table index */
} Elf32_Ehdr;

/* Fields in the e_ident array. */
#define EI_MAG0		0			/* File identificatino byte 0 index */
#define ELFMAG0		0x7f		/* Magic number byte 0 */

#define EI_MAG1		1			/* File identificatino byte 1 index */
#define ELFMAG1		'E'			/* Magic number byte 1 */

#define EI_MAG2		2			/* File identificatino byte 2 index */
#define ELFMAG2		'L'			/* Magic number byte 2 */

#define EI_MAG3		3			/* File identificatino byte 3 index */
#define ELFMAG3		'F'			/* Magic number byte 3 */

#define EI_CLASS	4			/* File class byte index */
#define ELFCLASS32	1			/* 32-bit objects */

#define EI_DATA		5			/* Data encoding byte index */
#define ELFDATA2LSB	1			/* 2's complement, little endian */

#define EI_VERSION	6			/* File version byte index */
								/* Value must be EV_CURRENT */

#define EI_OSABI	7			/* OS ABI identification */
#define ELFOSABI_NONE	0		/* UNIX System V ABI */
#define ELFOSABI_SYSV	0		/* Alias. */

#define EI_ABIVERSION	8		/* ABI version */

#define EI_PAD		9			/* Byte index of padding bytes */

/* Legal values for e_type (object file type). */
#define ET_NONE		0			/* No file type */
#define ET_REL		1			/* Relocatable file */
#define ET_EXEC		2			/* Executable file */
#define ET_DYN		3			/* Shared object file */
#define ET_CORE		4			/* Core file */

/* Legal values for e_machine (architecture). */
#define EM_386		3			/* Intel 80386 */

/* Legal values for e_version (version). */
#define EV_NONE		0			/* Invalid ELF version */
#define EV_CURRENT	1			/* Current version */

/* Section header. */
typedef struct {
	Elf32_Word	sh_name;		/* Section name (string tbl index) */
	Elf32_Word	sh_type;		/* Section type */
	Elf32_Word	sh_flags;		/* Section flags */
	Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
	Elf32_Off	sh_offset;		/* Section file offset */
	Elf32_Word	sh_size;		/* Section size in bytes */
	Elf32_Word	sh_link;		/* Link to another section */
	Elf32_Word	sh_info;		/* Additional section information */
	Elf32_Word	sh_addralign;	/* Section alignment */
	Elf32_Word	sh_entsize;		/* Entry size if section holds table */
} Elf32_Shdr;

/* Legal values for sh_type (section type). */
#define SHT_NULL	0			/* Section header table entry unused */

/* Program segment header */
typedef struct {
	Elf32_Word	p_type;			/* Segment type */
	Elf32_Off	p_offset;		/* Segment file offset */
	Elf32_Addr	p_vaddr;		/* Segment virtual address */
	Elf32_Addr	p_paddr;		/* Segment physical address */
	Elf32_Word	p_filesz;		/* Segment size in file */
	Elf32_Word	p_memsz;		/* Segment size in memory */
	Elf32_Word	p_flags;		/* Segment flags */
	Elf32_Word	p_align;		/* Segment alignment */
} Elf32_Phdr;

/* Legal values for p_type (segment type). */
#define PT_NULL		0			/* Program header table entry unused. */
#define PT_LOAD		1			/* Loadable program segment */
#define	PT_DYNAMIC	2			/* Dynamic linking information */
#define PT_INTERP	3			/* Program interpreter */
#define PT_NOTE		4			/* Auxiliary information */
#define PT_SHLIB	5			/* Reserved */
#define PT_PT_PHDR	6			/* Entry for header table itself */
#define PT_GNU_STACK	0x6474e551	/* Indicates stack executability */

/* Legal values for p_flags (segment flags). */
#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */

#endif
