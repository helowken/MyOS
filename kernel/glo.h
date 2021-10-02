#ifndef	GLO_H
#define	GLO_H

#ifdef	_TABLE
#undef	EXTERN
#define	EXTERN
#endif

#include "minix/config.h"
#include "config.h"

/* Kernel information structures. This groups vital kernel information. */
EXTERN phys_bytes imgHdrPos;	/* Address of image headers. */
EXTERN KernelInfo kernelInfo;	/* Kernel information for users. */

extern SegDesc gdt[];			/* GDT (global descriptor table). */

#endif
