#include "code.h"
#include "stdarg.h"
#include "util.h"
#include "boot.h"

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
	
		for (i = 0; i < 3; ++i) {
			if (i == 0 || mem[i].base != 0) 
			  printf("Mme[%d] base: 0x%08x, size: %d B\n", i, mem[i].base, mem[i].size);
		}
	}
}

static void initialize() {
	u32_t memEnd, newAddr, dma64k, runSize, oldAddr;

	runSize = (u32_t) &end;
	runSize += STACK_SIZE;

	oldAddr = caddr;
	memEnd = mem[0].base + mem[0].size;
	newAddr = (memEnd - runSize) & ~0x0000FL;
	dma64k = (memEnd - 1) & ~0x0FFFFL;

	if (newAddr < dma64k) 
	  newAddr = dma64k - runSize;

	caddr = newAddr;

	printf("caddr: 0x%x\n", &oldAddr);
	printf("%x,%x,%x\n", newAddr, oldAddr, runSize);

	rawCopy(newAddr, oldAddr, runSize); 
	printf("============\n");
}

void boot() {
	//int i;

	determineAvailableMemory();
	initialize();

	/*
	for (i = 0; i < 48; ++i) {
		if (i % 8 == 0)
			printf("\n");
		printByteHex(x_gdt[i]);
		printf(" ");
	}
	*/
}

