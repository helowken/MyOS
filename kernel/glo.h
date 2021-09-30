#ifndef	_GLO_H
#define	_GLO_H

#ifdef	_TABLE
#undef	EXTERN
#define	EXTERN
#endif

#include "minix/config.h"

extern SegDesc gdt[];			/* GDT (global descriptor table) */

#endif
