#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

typedef unsigned long	ino_t;		// i-node number of file system
typedef unsigned long	off_t;		// offset within a file

typedef unsigned char	u8_t;
typedef unsigned short	u16_t;
typedef unsigned long	u32_t;

typedef char			i8_t;
typedef short			i16_t;
typedef long			i32_t;

typedef struct { u32_t _[2]; } u64_t;

#endif
