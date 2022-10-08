#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long	clock_t;			/* Unit for system accounting */
#endif

#ifndef _SIZE_T
#define	_SIZE_T 
typedef unsigned int size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef int ssize_t;
#endif

#ifndef _SIGSET_T
#define _SIGSET_T
typedef unsigned long	sigset_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;				/* Time in sec since 1 Jan 1970 0000 GMT */
#endif

typedef long	suseconds_t;		/* Time in microseconds */

/* Types used in disk, inode, etc. data structures. */
typedef short			dev_t;		/* Holds (major|minor) device pair */
typedef int				pid_t;		/* Process id (must be signed) */
typedef	short			uid_t;		/* User id */
typedef char			gid_t;		/* Group id */
typedef unsigned long	zone_t;		/* Zone number */
typedef unsigned long	block_t;	/* block number */
typedef unsigned long	bit_t;		/* Bit number in a bit map */
typedef unsigned long	ino_t;		/* i-node number of file system */
typedef unsigned short	mode_t;		/* File type and permissions bits */
typedef short			nlink_t;	/* Number of links to a file */
typedef unsigned long	off_t;		/* Offset within a file */
typedef unsigned short	bitchunk_t;	/* Collection of bits in a bitmap */

typedef unsigned char	u8_t;
typedef unsigned short	u16_t;
typedef unsigned long	u32_t;

typedef char			i8_t;
typedef short			i16_t;
typedef long			i32_t;

typedef struct { u32_t _[2]; } u64_t;

typedef int				_mnx_Mode_t;

/* To avoid confliting with Linux types */
typedef off_t			Off_t;		
typedef mode_t			Mode_t;
typedef time_t			Time_t;
typedef zone_t			Zone_t;
typedef block_t			Block_t;
typedef bit_t			Bit_t;
typedef ino_t			Ino_t;
typedef nlink_t			Nlink_t;
typedef bitchunk_t		Bitchunk_t;
typedef dev_t			Dev_t;
typedef uid_t			Uid_t;
typedef gid_t			Gid_t;

/* Signal handler type, e.g. SIG_IGN */
typedef void (*sighandler_t)(int);

#endif
