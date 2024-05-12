#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define offsetof(type, ident) ((size_t) (unsigned long) &((type *)0)->ident)

typedef long ptrdiff_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;	/* Type returned by sizeof */
#endif

#ifndef _WCHAR_T
#define	_WCHAR_T
typedef char wchar_t;	/* Type expanded character set */
#endif

#endif
