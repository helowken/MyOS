#include <sys/ioctl.h>
#include <linux/fs.h>
#include "common.h"


int main(int argc, char **argv) {
	int fd;
	size_t size;
	char *progName, *device;
	int partIdx;
	PartitionEntry table[NR_PARTITIONS];

	progName = getProgName(argv);

	if (argc < 3)
	  usageErr("%s device partIdx\n", progName);

	device = argv[1];
	partIdx = getPartIdx(argv[2]);
	fd = ROpen(device);
	getPartitionTable(device, fd, PART_TABLE_OFF, table);
	if (table[partIdx].type == MINIX_PART)
	  size = table[partIdx].sectorCount * SECTOR_SIZE;
	else 
	  size = 0;
    printf("%u\n", size);
	return 0;
}
