#ifndef _MINIX_COM_H
#define _MINIX_COM_H

#define arraySize(a)		(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)		((a) + arraySize(a))

/* Numer of tasks. Note that NR_PROCS is defined in <minix/config.h>. */
#define	NR_TASKS		4

#endif
