#include "code.h"
#include "util.h"

const char *destE820 = (char *) 0x8000;
static char hexBuf[HEX_BUF_LEN]; 

void printLowMem() {
	int memSize;
	memSize = detectLowMem();
	if (memSize >= 0)
	  printf("Low Memory: %d KB\n", memSize);
}

void print88Mem() {
	int memSize;
	memSize = detect88Mem();
	if (memSize >= 0)
	  printf("88 Memory: %d KB\n", memSize);
}

void printE820Mem() {
	int i, count;

	count = detectE820Mem();
	if (count > 0) {
		printf("E820 Memory Entries: %d\n", count);

		E820MemEntry entries[count];
		memcpy(entries, destE820, sizeof(E820MemEntry) * count);

		printf("%-20s%-20s%-8s%8s\n", "Base", "Length", "Type", "ExAttrs");
		for (i = 0; i < count; ++i) {
			printf("%08x%08x    %08x%08x    %4d    %8d\n", 
						entries[i].base._[1], 
						entries[i].base._[0],
						entries[i].len._[1],
						entries[i].len._[0],
						entries[i].type,
						entries[i].exAttrs);
		}
	}
}

void printE801Mem() {
	int low = 0, high = 0;	
	if (detectE801Mem(&low, &high, true) == 0)
	  printf("Extended mem between 1-16M: %d (K), above 16M: %d (64K)\n", low, high);
}

char * 
num2Hex(int num, int len, bool withPrefix) {
	int i, j;
	int n;

	i = len - 1;
	j = 0;
	if (withPrefix) {
		hexBuf[0] = '0';
		hexBuf[1] = 'x';
		j = 2;
	}

	for (; i >= 0 && j < HEX_BUF_LEN; --i, ++j) {
		n = (num >> (i * 4)) & 0xF;
		n += 0x30;
		if (n > 0x39)
		  n += 7;
		hexBuf[j] = n;
	}
	for (; j < HEX_BUF_LEN; ++j) {
		hexBuf[j] = '\0';
	}
	return hexBuf;
}

void printSepLine() {
	printf("-------------------------\n");
}

void printNumHex(int num, int len) {
	print(num2Hex(num, len, true));
}

void printlnNumHex(int num, int len) {
	println(num2Hex(num, len, true));
}

char * 
byte2Hex(char num) {
	return num2Hex(num, BYTE_BITS, false);
}

char * 
short2Hex(short num) {
	return num2Hex(num, SHORT_BITS, false);
}

char * 
int2Hex(int num) {
	return num2Hex(num, INT_BITS, false);
}

void printByteHex(char num) {
	printNumHex(num, BYTE_BITS);
}

void printlnByteHex(char num) {
	printlnNumHex(num, BYTE_BITS);
}

void printShortHex(short num) {
	printNumHex(num, SHORT_BITS);
}

void printlnShortHex(short num) {
	printlnNumHex(num, SHORT_BITS);
}

void printIntHex(int num) {
	printNumHex(num, INT_BITS);
}

void printlnIntHex(int num) {
	printlnNumHex(num, INT_BITS);
}

void printRangeHex(char *pos, int num, int col) {
	int i;
	for (i = 0; i < num; ++i) {
		if (i > 0 && i % col == 0)
			printf("\n");
		printByteHex(pos[i]);
		printf(" ");
	}
	printf("\n");
}


