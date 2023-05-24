#ifndef _S_I_DISK_H
#define _S_I_DISK_H

#include <minix/ioctl.h>

#define DIOC_SET_PART		_IOW('d', 3, Partition)
#define DIOC_GET_PART		_IOR('d', 4, Partition)
#define DIOC_TIMEOUT		_IOW('d', 6, int)
#define DIOC_OPEN_COUNT		_IOR('d', 7, int)

#endif
