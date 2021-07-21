#include "code16gcc.h"
#include "stdarg.h"
#include "util.h"

void boot() {
	printf("%s, %d, 0x%x, %s, %c\n", "abc", 333, 0x9876ABCD, "xxx-yyy", 'a');
	printf("%d, 0x%x\n", 0x1234, 0x1234);

	printf("=========================\n");
	printf("Hello, world!\n");
	printf("aaa -- xxx\n");

	printf("0x%x\n", 0x1234);
	printf("0x%X\n", 0xabcd9876);

	printf("%04X\n", 0xF);
	printf("0x%x\n", 0xaa55);
	printf("0x%x\n", 0x11112222);
	printf("=========================\n");
}

