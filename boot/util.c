#include "code.h"
#include "sys/types.h"
#include "limits.h"
#include "util.h"

const char *destE820 = (char *) 0x8000;
static char hexBuf[HEX_BUF_LEN]; 


bool numPrefix(char *s, char **ps) {
	char *n = s;
	while (between('0', *n, '9')) {
		++n;
	}
	if (n == s)
	  return false;
	if (ps == NULL)
	  return *n == 0;

	*ps = n;
	return true;
}

bool numeric(char *s) {
	return numPrefix(s, (char **) NULL);
}

char *ul2a(u32_t n, unsigned b) {
/* Transform a long number to ascii at base b, (b >= 8). */
	static char num[(CHAR_BIT * sizeof(n) + 2) / 3 + 1];
	char *a = arrayLimit(num) - 1;
	static char hex[16] = "0123456789ABCDEF";
	do {
		*--a = hex[n % b];	
	} while ((n/=b) > 0);
	return a;
}

char *ul2a10(u32_t n) {
/* Transform a long number to ascii at base 10. */
	return ul2a(n, 10);
}

long a2l(char *a) {
	int sign = 1;
	int n = 0;
	if (*a == '-') {
		sign = -1;
		++a;
	}
	while (between('0', *a, '9')) {
		n = n * 10 + (*a++ - '0');
	}
	return n * sign;
}

unsigned a2x(char *a) {
/* Ascii to hex. */
	unsigned n = 0;
	int c;

	while (true) {
		c = *a;
		if (between('0', c, '9'))
		  c = c - '0' + 0x0;
		else if (between('a', c, 'f'))
		  c = c - 'a' + 0xa;
		else if (between('A', c, 'F'))
		  c = c - 'A' + 0xA;
		else 
		  break;
		n = (n << 4) | c;
		++a;
	}
	return n;
}

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

char *num2Hex(int num, int len, bool withPrefix) {
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

char *byte2Hex(char num) {
	return num2Hex(num, BYTE_BITS, false);
}

char *short2Hex(short num) {
	return num2Hex(num, SHORT_BITS, false);
}

char *int2Hex(int num) {
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


