#include "code.h"
#include "util.h"

static char hexBuf[HEX_BUF_LEN]; 

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
