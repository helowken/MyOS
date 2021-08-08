#include "code.h"
#include "stdarg.h"
#include "util.h"
#include "boot.h"
#include "partition.h"

extern char x_gdt[48];

void test() {
	printf("========== Test: printf ===============\n");
	printf("etext: 0x%x, edata: 0x%x, end: 0x%x\n", &etext, &edata, &end);
	printf("%s, %d, 0x%x, %s, %c, 0x%X\n", "abc", 333, 0x9876ABCD, "xxx-yyy", 'a', 0xabcd9876);
	printf("%x, %04X, 0x%x, 0x%x\n", 0xA, 0xF,0xaa55, 0x11112222);

	printf("========== Test: print..Hex ===============\n");
	printByteHex(0xA1);
	print("    ");
	printShortHex(0xA1B2);
	print("    ");
	printlnIntHex(0xA1B2C8D9);
	
	printf("========== Test: Memory Detection ===============\n");
	printLowMem();
	print88Mem();
	printE801Mem();
	printE820Mem();

	println("========== Test end ===============\n");

}

static void determineAvailableMemory() {
	int i, memSize, low, high;

	if ((memSize = detectLowMem()) >= 0) {
		mem[0].base = 0;
		mem[0].size = memSize << 10;

		if (detectE801Mem(&low, &high, true) == 0) {
			mem[1].base = 0x100000;
			mem[1].size = low << 10;

			// if adjacent
			if (low == (15 << 10)) {
				mem[1].size += high << 16;	
			} else {
				mem[2].base = 0x1000000;
				mem[2].size = high << 16;
			}
		} else if ((memSize = detect88Mem()) >= 0) {
			mem[1].base = 0x100000;
			mem[1].size = memSize << 10;
		}
		if (DEBUG) {
			for (i = 0; i < 3; ++i) {
				if (i == 0 || mem[i].base != 0) 
				  printf("Mme[%d] base: 0x%08x, size: %d B\n", i, mem[i].base, mem[i].size);
			}
		}
	}
}

static void copyToFarAway() {
	u32_t memEnd, newAddr, dma64k, oldAddr;

	oldAddr = caddr;
	memEnd = mem[0].base + mem[0].size;
	newAddr = (memEnd - runSize) & ~0x0000FL;
	dma64k = (memEnd - 1) & ~0x0FFFFL;

	/* Check if code and data segment cross a 64K boundary. */
	if (newAddr < dma64k) 
	  newAddr = dma64k - runSize;

	/* Keep pc concurrent. */
	newAddr = newAddr & ~0xFFFFL;

	/* Set the new caddr for relocate. */
	caddr = newAddr;

	/* Copy code and data. */
	rawCopy((char *) newAddr, (char *) oldAddr, runSize); 

	/* Make the copy running. */
	relocate();
}

/*
static int getMaster(char *master, struct partitionEntry **table, u32_t pos) {
	int r, n;
	struct partitionEntry *pe, **pt;
	if ((r = readSectors(mon2Abs(master), pos, 1)) != 0)
	  return r;
	return 0;
}
*/

static void initialize() {
	//char master[SECTOR_SIZE];
	//struct partitionEntry *table[NR_PARTITIONS];
	//u32_t masterPos;

	copyToFarAway();

	if (device < 0x80) {
		printf("Device is not a hard disk.\n");
		return;
	}

	rawCopy(mon2Abs(&lowSector),
		vec2Abs(&bootPartEntry) + offsetof(struct partitionEntry, lowSector),
		sizeof(lowSector));

	printf("low sector: %x\n", lowSector);

	printf("============\n");
}

void boot() {
	printf("device addr: %x, device: %x\n", &device, device);
	determineAvailableMemory();
	initialize();
	if (DEBUG) {
		test();
		printRangeHex((char *) &x_gdt, 48, 8);
	}
	printf("device: 0x%x\n", device);
}

