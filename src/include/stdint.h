#ifndef _STDINT_H
#define _STDINT_H

#ifndef _MINIX__TYPES_H
#include <sys/types.h>
#endif

typedef i8_t	int8_t;
typedef i16_t	int16_t;
typedef i32_t	int32_t;

typedef u8_t	uint8_t;
typedef u16_t	uint16_t;
typedef u32_t	uint32_t;
typedef u64_t	uint64_t;

#endif
