#ifndef _MINIX__PARTITION_H
#define	_MINIX__PARTITION_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

typedef struct {
	u64_t base;		/* Byte offset to the partition start */
	u64_t size;		/* Number of bytes in the partition */
	unsigned cylinders;		/* Disk geometry */
	unsigned heads;
	unsigned sectors;
} Partition;

#endif
