#include <sys/ioctl.h>
#include <linux/fs.h>
#include "common.h"

/* Extra flags at construction time */
#define EXPAND_FLAG	0x01	/* Add the remaining sectors to this one */
#define EXIST_FLAG	0x02

static char *progName;
static bool showPrimary = false, showSubPart = false, quiet = false;
static int nPart = 0;
static PartitionEntry selectedPart, table[2 * NR_PARTITIONS + 1];
static char *device;
static int partIdx;
static const unsigned heads = 255;
static const unsigned sectors = 63;

static void usage() {
	usageErr("%s [-psq] device partIdx [type:]length[+*]\n", progName);
}

static void parse(char *desc) {
	int seen = 0, type, flags, c;
	unsigned long sectorCount;

	if (strchr(desc, ':') == NULL) {
		/* A hole. */
		if (nPart % 2 != 0) {
			fprintf(stderr, "%s: Two holes can't be adjacent.\n", progName);
			exit(EXIT_FAILURE);
		}
		type = NO_PART;
		seen |= 1;
	} else {
		/* A partition. */
		if (nPart % 2 == 0) {
			/* Need a hole before this partition. */
			if (nPart == 0) {
				/* First hole contains the partition table. */
				table[0].sectorCount = 1;
			}
			++nPart;
		}
		type = NO_PART;
		while (true) {
			c = *desc++;
			if (between('0', c, '9'))
			  c = (c - '0') + 0x0;
			else if (between('a', c, 'z'))
			  c = (c - 'a') + 0xa;
			else if (between('A', c, 'Z'))
			  c = (c - 'A') + 0xA;
			else
			  break;
			type = 0x10 * type + c;
			seen |= 1;
		}
		if (c != ':')
		  usage();
	}

	flags = 0;
	sectorCount = 0;
	while (between('0', (c = *desc++), '9')) {
		sectorCount = 10 * sectorCount + (c - '0');
		seen |= 2;
	}

	while (true) {
		if (c == '*') 
		  flags |= ACTIVE_FLAG;
		else if (c == '+' && !(flags & EXIST_FLAG))
		  flags |= EXPAND_FLAG;
		else
		  break;
		c = *desc++;
	}

	if (seen != 3 || c != 0)
	  usage();

	if (nPart >= arraySize(table)) 
	  fatal("%s: too many partitions, only %d possible.\n", 
				  progName, NR_PARTITIONS);

	table[nPart].status = flags;
	table[nPart].type = type;
	table[nPart].sectorCount = sectorCount;
	++nPart;
}

static void sec2Dos(unsigned long sec, unsigned char *dos) {
	unsigned secPerCyl = heads * sectors;
	unsigned cyl;

	cyl = sec / secPerCyl;
	dos[2] = cyl;
	dos[1] = ((sec % sectors) + 1) | ((cyl >> 2) & 0xC0);
	dos[0] = (sec % secPerCyl) / sectors;
}

static void showCHS(unsigned long pos) {
	unsigned cyl, head, sec;

	if (pos == -1) {
		cyl = head = 0;
		sec = -1;
	} else {
		cyl = pos / (heads * sectors);
		head = (pos / sectors) - (cyl * heads);
		sec = pos % sectors;
	}
	printf("  %4d/%03d/%02d", cyl, head, sec);
}

static void showPart(PartitionEntry *p, int n) {
	static bool banner = false;

	if (!banner) {
		printf("Part    CHS First     CHS Last     Start   Sectors       Kb\n");
		banner = true;
	}

	printf("%3d%s", n, (p->status & ACTIVE_FLAG) ? "*" : " ");
	showCHS(p->lowSector);
	showCHS(p->lowSector + p->sectorCount - 1);
	printf("  %8u  %8u  %7u\n", p->lowSector, p->sectorCount, 
				p->sectorCount / 2);
}

static void distribute() {
/* Fit the partitions onto the device. Try to start and end them on a
 * cylinder boundary if so required. The first partition is to start on
 * track 1, not on cylinder 1.
 */
	PartitionEntry *pe, *exp;
	unsigned long base, oldBase, count;
	int i;

	do {
		exp = NULL;
		base = selectedPart.lowSector;
		count = selectedPart.sectorCount;

		for (pe = table; pe < arrayLimit(table); ++pe) {
			oldBase = base;
			pe->lowSector = base;
			if (pe->status & EXPAND_FLAG)
			  exp = pe;
			base = pe->lowSector + pe->sectorCount;
			count -= base - oldBase;
		}
		if (count < 0) 
		  fatal("%s: %s is %ld sectors too small\n", 
					  progName, device, -count);
		if (exp != NULL) {
			/* Add leftover space to the partition marked for 
			 * expanding. 
			 * */
			exp->sectorCount += count;
			exp->status &= ~EXPAND_FLAG;
		}
	} while (exp != NULL);

	for (i = 0, pe = table; pe < arrayLimit(table); ++pe, ++i) {
		if (pe->type == NO_PART) {
			memset(pe, 0, sizeof(*pe));
		} else {
			sec2Dos(pe->lowSector, &pe->startHead);
			sec2Dos(pe->lowSector + pe->sectorCount - 1, &pe->lastHead);
			pe->status &= ACTIVE_FLAG;
		}
		if (i % 2 > 0 && !quiet)
		  showPart(pe, (i - 1) / 2);
	}
}

static void geometry() {
	int fd;
	int i;
	PartitionEntry table[NR_PARTITIONS];

	fd = ROpen(device);
	getPartitionTable(device, fd, PART_TABLE_OFF, table);
	selectedPart = table[partIdx];

	if (showPrimary) {
		printf("\n");
		printf("==== Partition %d Table ====\n", partIdx);
		for (i = 0; i < NR_PARTITIONS; ++i) {
			showPart(&table[i], i);
		}
	}

	if (showSubPart) {
		printf("\n");
		memset(table, 0, sizeof(table));
		getPartitionTable(device, fd, OFFSET(selectedPart.lowSector) + PART_TABLE_OFF, table);
		printf("==== Sub Partition %d Table ====\n", partIdx);
		for (i = 0; i < NR_PARTITIONS; ++i) {
			showPart(&table[i], i);
		}
	}
	printf("\n");

	Close(device, fd);
}

static void writeTable() {
	int fd;
	uint16_t signature = 0xAA55;
	PartitionEntry newTable[NR_PARTITIONS];
	int i;

	for (i = 0; i < NR_PARTITIONS; ++i) {
		newTable[i] = table[i * 2 + 1];
	}

	fd = WOpen(device);
	Lseek(device, fd, OFFSET(selectedPart.lowSector) + PART_TABLE_OFF);
	Write(device, fd, (char *) newTable, sizeof(newTable));
	Write(device, fd, (char *) &signature, sizeof(signature));
	Close(device, fd);
}

int main(int argc, char **argv) {
	int i;
	int opt;

    progName = getProgName(argv);

	while ((opt = getopt(argc, argv, "psq")) != EOF) {
		switch (opt) {
			case 'p':
				showPrimary = true;
				break;
			case 's':
				showSubPart = true;
				break;
			case 'q':
				quiet = true;
				break;
			default:
				usage();
		}
	}
	i = optind;

	if (argc - i < 2)
	  usage();

	device = argv[i++];
	partIdx = getPartIdx(argv[i++]);
	geometry();
	while (i < argc) {
		parse(argv[i++]);
	}

	if (!showPrimary && !showSubPart) {
		distribute();
		writeTable();
	}
	exit(EXIT_SUCCESS);
}
