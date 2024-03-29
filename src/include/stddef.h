#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define offsetof(type, ident) ((size_t) (unsigned long) &((type *)0)->ident)

typedef long ptrdiff_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

#endif
