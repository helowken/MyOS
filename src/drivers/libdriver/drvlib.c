#include "driver.h"
#include "drvlib.h"
#include <unistd.h>

#define extPart(s)		((s) == 0x05 || (s) == 0x0F)	/* Extended partition? */

static void sort(PartitionEntry *table) {
/* Sort a partition table. */
	PartitionEntry *pe, tmp;
	int n = NR_PARTITIONS;

	do {
		for (pe = table; pe < table + NR_PARTITIONS - 1; ++pe) {
			if (pe[0].type == NO_PART || 
				(pe[0].lowSector > pe[1].lowSector && pe[1].type != NO_PART)) {
				tmp = pe[0];
				pe[0] = pe[1];
				pe[1] = tmp;
			}
		}
	} while (--n > 0);
}

static int getPartitionTable(Driver *dp, int device,
			unsigned long offset, PartitionEntry *table) {
/* Read the partition table for the device, return true iff there were no 
 * errors. 
 */
	IOVec iov;
	off_t position;
	static unsigned char partBuf[CD_SECTOR_SIZE];

	position = offset << SECTOR_SHIFT;
	iov.iov_addr = (vir_bytes) partBuf;
	iov.iov_size = CD_SECTOR_SIZE;
	if ((*dp->drPrepare)(device) != NIL_DEV)
	  (*dp->drTransfer)(SELF, DEV_GATHER, position, &iov, 1);

	if (iov.iov_size != 0)
	  return 0;
	if (partBuf[510] != 0x55 || partBuf[511] != 0xAA) {
		/* Invalid partition table . */
		return 0;
	}
	memcpy(table, partBuf + PART_TABLE_OFF, NR_PARTITIONS * sizeof(table[0]));
	return 1;
}

static void extendedPartition(Driver *dp, int extDev, unsigned long extBase) {
	// TODO
}

void partition(Driver *dp, int device, int style) {
/* This routine is called on first open to initialize the partition tables
 * of a device. It makes sure that each partition falls safely within the 
 * device's limits. Depending on the partition style we are either making
 * floppy partitions, primary partitions or subpartitions. Only primary
 * systems that expect this.
 */
	PartitionEntry table[NR_PARTITIONS], *pe;
	Device *dv;
	int disk, part;
	unsigned long base, limit, partLimit;

	/* Get the geometry of the device to partition. */
	if ((dv = (*dp->drPrepare)(device)) == NIL_DEV || 
		cmp64u(dv->dv_size, 0) == 0)
	  return;

	base = div64u(dv->dv_base, SECTOR_SIZE);
	limit = base + div64u(dv->dv_size, SECTOR_SIZE);

	/* Read the partition table for the device. */
	if (!getPartitionTable(dp, device, 0L, table))  
	  return;

	/* Compute the device number of the first partition. */
	switch (style) {
		case P_FLOPPY:
			device += MINOR_fd0p0;
			break;
		case P_PRIMARY:
			sort(table);		
			device += 1;	/* Skip the 0th partition which is logical for the total drive. */
			break;
		case P_SUB:
			disk = device / DEV_PER_DRIVE;
			part = device % DEV_PER_DRIVE - 1;
			device = MINOR_d0p0s0 + (disk * NR_PARTITIONS + part) * NR_PARTITIONS;
			break;
	}

	/* Find an array of devices. */
	if ((dv = (*dp->drPrepare)(device)) == NIL_DEV)
	  return;

	/* Set the geometry of the partitions from the partition table. */
	for (part = 0; part < NR_PARTITIONS; ++part, ++dv) {
		/* Shrink the partition to fit within the device. */
		pe = &table[part];
		partLimit = pe->lowSector + pe->sectorCount;
		if (partLimit < pe->lowSector || partLimit > limit)
		  partLimit = limit;
		if (pe->lowSector < base)
		  pe->lowSector = base;
		if (partLimit < pe->lowSector)
		  partLimit = pe->lowSector;

		dv->dv_base = mul64u(pe->lowSector, SECTOR_SIZE);
		dv->dv_size = mul64u(partLimit - pe->lowSector, SECTOR_SIZE);

		if (style == P_PRIMARY) {
			/* Each Minix primary partition can be sub-partitioned. */
			if (pe->type == MINIX_PART)  
			  partition(dp, device + part, P_SUB);

			/* An extended partition has logical partitions. */
			if (extPart(pe->type))
			  extendedPartition(dp, device + part, pe->lowSector);
		}
	}
}

