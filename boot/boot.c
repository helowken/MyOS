#include "code.h"
#include "stdarg.h"
#include "util.h"

static void test() {
	printf("========== Test: printf ===============\n");
	printf("Hello, world!\n");
	printf("%s, %d, 0x%x, %s, %c, 0x%X\n", "abc", 333, 0x9876ABCD, "xxx-yyy", 'a', 0xabcd9876);
	printf("%04X, 0x%x, 0x%x\n", 0xF,0xaa55, 0x11112222);

	printf("========== Test: print..Hex ===============\n");
	printByteHex(0xA1);
	print("    ");
	printShortHex(0xA1B2);
	print("    ");
	printlnIntHex(0xA1B2C8D9);
	
	printf("========== Test: Memory Detection ===============\n");
	printLowMem();
	printE820Mem();

	println("========== Test end ===============\n");
}

void boot() {
	test();
}

