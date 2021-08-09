#ifndef _PARTITION_H
#define _PARTITION_H

struct partitionEntry {
	u8_t status;
	u8_t startHead;
	u8_t startSector;
	u8_t startCylinder;
	u8_t type;
	u8_t lastHead;
	u8_t lastSector;
	u8_t lastCylinder;
	u32_t lowSector;
	u32_t sectorCount;
};

#define NR_PARTITIONS		4		// Number of entries in partition table
#define PART_TABLE_OFF		0x1BE	// Offset of partition table in boot sector

/* Partition types */
#define INACTIVE_PART		0x00	// Inactive entry
#define ACTIVE_PART			0x01	// Active entry

#endif
