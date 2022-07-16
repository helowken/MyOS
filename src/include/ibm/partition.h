#ifndef _PARTITION_H
#define _PARTITION_H

#include "stdint.h"

typedef struct {
	uint8_t status;			/* Boot indicator 0/ACTIVE_FLAG */
	uint8_t startHead;		/* Head value for first sector */
	uint8_t startSector;	/* Sector value + cylinder (2)bits for first sector */
	uint8_t startCylinder;	/* Track value for first sector */
	uint8_t type;			/* System indicator */
	uint8_t lastHead;		/* Head value for last sector */
	uint8_t lastSector;		/* Sector value + cylinder (2)bits for last sector */
	uint8_t lastCylinder;	/* Track value for last sector */
	uint32_t lowSector;		/* Logical first sector */
	uint32_t sectorCount;	/* Size of partition in sectors */
} PartitionEntry;

#define ACTIVE_FLAG			0x80	/* Value for active in status field (hd0) */
#define NR_PARTITIONS		4		/* Number of entries in partition table */
#define PART_TABLE_OFF		0x1BE	/* Offset of partition table in boot sector */

/* Partition types. */
#define NO_PART				0x00	/* Unused entry */
#define MINIX_PART			0x81	/* Minix partition type */

#define BOOTABLE(p)			((p)->status & 0x80)	/* Is it bootable */

#endif
