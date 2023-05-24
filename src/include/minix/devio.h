#ifndef _DEVIO_H
#define _DEVIO_H

#include <minix/sys_config.h>
#include <sys/types.h>

typedef u16_t port_t;

typedef struct { u16_t port;  u8_t value; } PvBytePair;
typedef struct { u16_t port; u16_t value; } PvWordPair;
typedef struct { u16_t port; u32_t value; } PvLongPair;

#define pvSet(pv, p, v)		((pv).port = (p), (pv).value = (v))
#define pvPtrSet(ptr, p, v)		((ptr)->port = (p), (ptr)->value = (v))

#endif
