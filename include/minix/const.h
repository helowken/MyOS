#ifndef _MINIX_CONST_H
#define _MINIX_CONST_H

#define EXTERN		extern	/* Used in *.h files */

#define HZ				60	/* clock freq (software settable on IBM-PC) */

#define NR_REMOTE_SEGS	3	/* Remote memory regions */

/* Memory related constants. */
#define NR_LOCAL_SEGS	3	/* Local segments per process (fixed) */
#define T				0	/* proc[i].p_memmap[T] is for text */
#define D				1	/* proc[i].p_memmap[D] is for data */
#define S				2	/* proc[i].p_memmap[S] is for stack */


/* Macros. */
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define MIN(a, b)		((a) < (b) ? (a) : (b))

#endif
