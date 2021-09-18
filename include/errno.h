#ifndef _ERRNO_H
#define _ERRNO_H

#ifdef	_SYSTEM
#	define _SIGN	-
#	define OK		0
#else
#	define _SIGN	
#endif

extern int errno;

#define	EIO			(_SIGN 5)	// input/output error
#define ENOEXEC		(_SIGN 8)	// exec format error
#define ENOMEM		(_SIGN 12)	// no enough space

#endif
