#ifndef _PARTITION_H
#define _PARTITION_H

typedef struct {
	uint8_t status;
	uint8_t startHead;
	uint8_t startSector;
	uint8_t startCylinder;
	uint8_t type;
	uint8_t lastHead;
	uint8_t lastSector;
	uint8_t lastCylinder;
	uint32_t lowSector;
	uint32_t sectorCount;
} PartitionEntry;

#define NR_PARTITIONS		4		/* Number of entries in partition table */
#define PART_TABLE_OFF		0x1BE	/* Offset of partition table in boot sector */

#define BOOTABLE(p)			((p)->status & 0x80)	/* Is it bootable */

#endif
