#ifndef _S_I_CMOS_H
#define _S_I_CMOS_H

#include <minix/ioctl.h>

#define CIOC_GET_TIME		_IOR('c', 1, u32_t)
#define CIOC_GET_TIME_Y2K	_IOR('c', 2, u32_t)
#define CIOC_SET_TIME		_IOW('c', 3, u32_t)
#define CIOC_SET_TIME_Y2K	_IOW('c', 4, u32_t)

#endif
