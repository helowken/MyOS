#ifndef _ERRNO_H
#define _ERRNO_H

#ifdef	_SYSTEM
#	define _SIGN	-
#	define OK		0
#else
#	define _SIGN	
#endif

extern int errno;

#define	EPERM		(_SIGN 1)	/* Operation not permitted */
#define ENOENT		(_SIGN 2)	/* No such file or directory */
#define ESRCH		(_SIGN 3)	/* No such process */
#define EINTR		(_SIGN 4)	/* Interrupted function call */
#define	EIO			(_SIGN 5)	/* Input/output error */
#define ENXIO		(_SIGN 6)	/* No such device or address */
#define E2BIG		(_SIGN 7)	/* Arg list too long */
#define ENOEXEC		(_SIGN 8)	/* Exec format error */
#define ECHILD		(_SIGN 10)	/* No child process */
#define EAGAIN		(_SIGN 11)	/* Resource temporarily unavailable */
#define ENOMEM		(_SIGN 12)	/* No enough space */
#define EACCES		(_SIGN 13)	/* Permission denied */
#define EFAULT		(_SIGN 14)	/* Bad address */
#define EBUSY		(_SIGN 16)	/* Resource busy */
#define ENODEV		(_SIGN 19)	/* No such device */
#define ENOTDIR		(_SIGN 20)	/* Not a directory */
#define EINVAL		(_SIGN 22)	/* Invalid argument */
#define ENFILE		(_SIGN 23)  /* Too many open files in system */
#define ENOTTY		(_SIGN 25)	/* Inappropriate I/O control operation */
#define ENOSPC		(_SIGN 28)	/* No space left on device */
#define EDOM		(_SIGN 33)	/* Domain error (from ANSI C std) */
#define ERANGE		(_SIGN 34)	/* Result too large */
#define ENOSYS		(_SIGN 38)	/* Function not implemented */

/* The following are not POSIX errors, but they can still happen.
 * All of these are generated by the kernel and relate to message passing.
 */
#define ELOCKED		(_SIGN 101) /* Can't send message due to deadlock. */
#define EBADCALL	(_SIGN 102)	/* Illegal system call number. */
#define EBADSRCDST	(_SIGN 103)	/* Bad source or destination process. */
#define ECALLDENIED	(_SIGN 104)	/* No permission for system call. */
#define EDEADDST	(_SIGN 105) /* Send destination is not alive. */
#define ENOTREADY	(_SIGN 106) /* Source or destination is not ready. */
#define EBADREQUEST	(_SIGN 107) /* Destination cannot handle request. */
#define EDONTREPLY	(_SIGN 201)	/* Pseudo-code: don't send a reply. */

#endif
