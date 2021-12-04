#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long	clock_t;	/* Unit for system accounting */
#endif

#ifndef _SIGSET_T
#define _SIGSET_T
typedef unsigned long	sigset_t;
#endif

typedef int				pid_t;		/* Process id (must be signed) */
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
