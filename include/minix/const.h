#ifndef _MINIX_CONST_H
#define _MINIX_CONST_H

#define EXTERN			extern		/* Used in *.h files */
#define PRIVATE			static		
#define PUBLIC						/* The opposite of PRIVATE */

typedef enum { false, true } bool;

#define NR_REMOTE_SEGS		3		/* Remote memory regions */

/* Macros. */
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define MIN(a, b)		((a) < (b) ? (a) : (b))

#endif
