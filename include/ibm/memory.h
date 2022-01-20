/* Physical memory layout. Design decisions made for the earliest PCs, caused
 * memory to be broken into the following four basic pieces:
 *	- Conventional or base memory: first 640 KB (incl. BIOS data, see below);
 *	  The top of conventional memory is often used by the BIOS to store data.
 *	- Upper Memory Area (UMA): upper 384 KB of the first megabyte of memory;
 *  - High Memory Area (HMA): first 64 KB of the second megabyte of memory;
 *  - Extended Memory: all the memory above first megabyte of memory.
 * The high memory area overlaps with the first 64 KB of extended memory, but
 * is different from the rest of extended memory because it can be accessed
 * when the processor is in real mode.
 */
#define BASE_MEM_BEGIN				0x000000
#define BASE_MEM_TOP				0x090000
#define BASE_MEM_END				0x09FFFF

#define UPPER_MEM_BEGIN				0x0A0000
#define UPPER_MEM_END				0x0FFFFF

#define HIGH_MEM_BEGIN				0x100000
#define	HIGH_MEM_END				0x10FFEF

#define EXTENDED_MEM_BEGIN			0x100000
#define EXTENDED_MEM_END			((unsigned) - 1)



/* The bottom part of conventional or base memory is occupied by BIOS data.
 * The BIOS memory can be distinguished in two parts:
 * o The first 1024 bytes of addressable memory contians the BIOS real-mode 
 *	 interrupt vector table (IVT). The table is used to access BIOS hardware
 *	 services in real-mode by loading a interrupt vector and issuing an INT
 *	 instruction. Some vectors contain BIOS data that can be retrieved 
 *	 directly and are useful in protected-mode as well.
 * o The BIOS data area is located directly above the interrupt vectors. It
 *   comprises 256 bytes of memory. These data are used by the device drivers
 *   to retrieve hardware details, such as I/O ports to be used.
 */
#define BIOS_MEM_BEGIN				0x00000		/* All BIOS memory */
#define BIOS_MEM_END				0x004FF		
#define	BIOS_IVT_BEGIN				0x00000		/* BIOS interrupt vectors */
#define BIOS_IVT_END				0x003FF		
#define BIOS_DATA_BEGIN				0x00400		/* BIOS data area */
#define BIOS_DATA_END
