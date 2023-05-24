#ifndef _SYS__SVRCTL_H
#define _SYS__SVRCTL_H

#ifndef _TYPES_H
#include <sys/types.h>
#endif

/* Server control commands have the same encoding as the commands for ioctls. */
#include <minix/ioctl.h>

/* MM controls. */
#define MM_SWAP_ON		_IOW('M', 5, MmSwapOn)
#define MM_SWAP_OFF		_IO ('M', 6)
#define MM_GET_PARAM	_IOW('M', 5, SysGetEnv)
#define MM_SET_PARAM	_IOR('M', 7, SysGetEnv)


typedef struct {
	u32_t offset;	/* Starting offset within file. */
	u32_t size;		/* Size of swap area. */
	char file[128];	/* Name of swap file/device. */
} MmSwapOn;

typedef struct {
	char *key;		/* Name requested. */
	size_t keyLen;	/* Length of name including \0. */
	char *val;		/* Buffer for returned data. */
	size_t valLen;	/* Size of returned data buffer. */
} SysGetEnv;

int svrctl(int request, void *data);

#endif
