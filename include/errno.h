#ifndef _ERRNO_H
#define _ERRNO_H

#ifdef	_SYSTEM
#	define _SIGN	-
#	define OK		0
#else
#	define _SIGN	
#endif

extern int errno;

#define	EIO			(_SIGN 5)	/* Input/output error */
#define ENOEXEC		(_SIGN 8)	/* Exec format error */
#define ENOMEM		(_SIGN 12)	/* No enough space */
#define ENOSPC		(_SIGN 28)	/* No space left on device */
#define ERANGE		(_SIGN 34)	/* Result too large */

/* The following are not POSIX errors, but they can still happen.
 * All of these are generated by the kernel and relate to message passing.
 */
#define ELOCKED		(_SIGN 101) /* Can't send message due to deadlock. */
#define EBADCALL	(_SIGN 102)	/* Illegal system call number. */
#define EBADSRCDST	(_SIGN 103)	/* Bad source or destination process. */
#define ECALLDENIED	(_SIGN 104)	/* No permission for system call. */
#define EDEADDST	(_SIGN 105) /* Send destination is not alive. */
#define ENOTREADY	(_SIGN 106) /* Source or destination is not ready. */
#define EDONTREPLY	(_SIGN 201)	/* Pseudo-code: don't send a reply. */

#endif
