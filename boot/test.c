#include "code.h"
#include "debug.h"
#include "stdarg.h"
#include "stdlib.h"
#include "limits.h"
#include "util.h"
#include "boot.h"
#include "partition.h"

#if DEBUG
static void testPrint() {
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

static void printPartitionEntry(struct partitionEntry **table) {
	struct partitionEntry **pt, *pe;

	printf("  Status  StHead  StSec  StCyl  Type  EdHead  EdSec  EdCyl  LowSec  Count\n");
	for (pt = table; pt < table + NR_PARTITIONS; ++pt) {
		pe = *pt;
		printf("  0x%-4x  %6d  %5d  %5d  %3xh  %6d  %5d  %5d  0x%04x  %5d\n", 
					pe->status, pe->startHead, pe->startSector, pe->startCylinder, 
					pe->type, pe->lastHead, pe->lastSector, pe->lastCylinder,
					pe->lowSector, pe->sectorCount);
	}
}

static void testMalloc() {
	typedef struct {
		char a1;
		short a2;
		int a3;
		long a4;
		char *a5;
	} AA;

	AA *pp[10], *p;
	for (int i = 0; i < 10; ++i) {
		p = malloc(sizeof(AA));
		p->a1 = 'a' + i;
		p->a2 = 1 + i;
		p->a3 = 10 + i;
		p->a4 = 100 + i;
		p->a5 = "abcdefg";
		pp[i] = p;
	}
	for (int i = 0; i < 10; ++i) {
		p = pp[i];
		printf("a1: %c, a2: %d, a3: %d, a4: %d, a5: %s\n",
			p->a1, p->a2, p->a3, p->a4, p-> a5);
	}
	for (int i = 0; i < 10; ++i) {
		p = realloc(pp[i], sizeof(AA));
		pp[i] = p;
		printf("a1: %c, a2: %d, a3: %d, a4: %d, a5: %s\n",
			p->a1, p->a2, p->a3, p->a4, p-> a5);
	}
	for (int i = 0; i < 10; ++i) {
		free(pp[i]);
	}
}
#endif
