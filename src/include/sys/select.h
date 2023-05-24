#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H

#include <sys/time.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>

/* Use this data type as basic storage unit in fd_set */
typedef u32_t fd_mask;

/* This many bits fit in an fd_set word. */
#define _FD_SETBITS_PER_WORD	(sizeof(fd_mask) * 8)

/* Bit manipulation macros */
#define _FD_BITMASK(b)	(1L <<((b) % _FD_SETBITS_PER_WORD))
#define _FD_BITWORD(b)	((b) / _FD_SETBITS_PER_WORD)

/* Default FD_SETSIZE is OPEN_MAX. */
#ifndef FD_SETSIZE
#define FD_SETSIZE	OPEN_MAX
#endif

/* We want to store FD_SETSIZE bits. */
#define _FD_SETWORDS ((FD_SETSIZE + _FD_SETBITS_PER_WORD - 1) / _FD_SETBITS_PER_WORD)

typedef struct {
	fd_mask fds_bits[_FD_SETWORDS];
} fd_set;

int select(int nfds, fd_set *readfds, fd_set *writefds, 
			fd_set *errorfds, struct timeval *timeout);

#define FD_ZERO(s)	\
do { int _i; \
	for(_i = 0; _i < _FD_SETWORDS; ++_i) {\
		(s)->fds_bits[_i] = 0; \
	}\
} while(0)
#define FD_SET(f, s)	\
do {\
	(s)->fds_bits[_FD_BITWORD(f)] |= _FD_BITMASK(f); \
} while (0)
#define FD_CLR(f, s)	\
do {\
	(s)->fds_bits[_FD_BITWORD(f)] &= ~(_FD_BITMASK(f)); \
} while (0)
#define FD_ISSET(f, s)	((s)->fds_bits[_FD_BITWORD(f)] & _FD_BITMASK(f))

/* Possible select() operation types; read, write, errors */
/* (FS/driver internal use only) */
#define SEL_RD		(1 << 0)
#define SEL_WR		(1 << 1)
#define SEL_ERR		(1 << 2)
#define SEL_NOTIFY	(1 << 3)	/* Not a real select operation */

#endif
