#include "code16gcc.h"

extern void display(char *str, int count, int row);

extern void print(int c);

static int calc(int n, int off) {
	return (((n >> off) & 0xFF) + 48) << off;
}

static int convert(int n) {
	int r;
	r = calc(n, 0) & 0xFF;
	r |= calc(n, 8) & 0xFF00;
	r |= calc(n, 16) & 0xFF0000;
	r |= calc(n, 24) & 0xFF000000;
	return r;
}


void main(const int row) {
	print(0x30303033);
	//display("xxxyyy", 6, row);
	//display("aabbccdd", 8, row + 1);
	//display("12345678", 8, row + 2);
}

void main2(const int row) {
	//print(convert(0x01020304));
	
	display("111111", 6, row);
	print(convert(row));

	//display("22222222", 8, row + 1);
	//display("33333333", 8, row + 2);
}
