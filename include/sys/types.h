#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#ifndef _CLOCK_T
#define _CLOCK_T
typedef unsigned long	clock_t;	/* Unit for system accounting */
#endif

#ifndef _SIGSET_T
#define _SIGSET_T
typedef unsigned long	sigset_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;				/* Time in sec since 1 Jan 1970 0000 GMT */
#endif

typedef short			dev_t;		/* Holds (major|minor) device pair */
typedef int				pid_t;		/* Process id (must be signed) */
typedef	short			uid_t;		/* User id */
typedef char			gid_t;		/* Group id */
typedef unsigned long	ino_t;		/* i-node number of file system */
typedef unsigned long	off_t;		/* Offset within a file */
typedef unsigned short	bitchunk_t;	/* Collection of bits in a bitmap */

typedef unsigned char	u8_t;
typedef unsigned short	u16_t;
typedef unsigned long	u32_t;

typedef char			i8_t;
typedef short			i16_t;
typedef long			i32_t;

typedef struct { u32_t _[2]; } u64_t;

typedef int				U8_t;
typedef int				U16_t;
typedef unsigned long	U32_t;

#endif
