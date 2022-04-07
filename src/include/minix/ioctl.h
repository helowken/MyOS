#ifndef _M_IOCTL_H
#define _M_IOCTL_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

/* Ioctls have the command encoded in the low-order word, and the size
 * of the parameter in the high-order byte. The 3 high bits of the high-
 * order word are used to encode the in/out/void status of the parameter.
 */
#define _IOC_PARAM_MASK		0x1FFF
#define _IOC_VOID			0x20000000
#define _IOC_IN				0x40000000
#define _IOC_OUT			0x80000000
#define _IOC_INOUT			(_IOC_IN | IOC_OUT)

/*
 * |...      |				_IOC_IN / _IOC_OUT / _IOC_INOUT
 * |. .... .... ....|		(sizeof(t) & _IOC_PARAM_MASK) << 16
 * |.... ....|				x << 8
 * |.... ....|				y
 */
#define _IO(x, y)			((x << 8) | y | _IOC_VOID)
#define _IOR(x, y, t)		((x << 8) | y | ((sizeof(t) & _IOC_PARAM_MASK) << 16) | _IOC_IN)
#define _IOW(x, y, t)		((x << 8) | y | ((sizeof(t) & _IOC_PARAM_MASK) << 16) | _IOC_OUT)
#define _IORW(x, y, t)		((x << 8) | y | ((sizeof(t) & _IOC_PARAM_MASK) << 16) | _IOC_INOUT)


int ioctl(int fd, int request, void *data);

#endif
