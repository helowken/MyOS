#include "code16gcc.h"
#include "stdarg.h"
#include "util.h"

void* va_derefSp(void *argp) {
	return derefSp(argp);
}

void test(char *fmt, ...) {
/*
	//printlnIntHex((int) fmt);
	//printlnIntHex((int) &fmt);
	printlnIntHex((int) &fmt);
	println(*(&fmt));

	char **pp = &fmt;
	char *p = derefSp(pp);
	//printlnIntHex((int) pp);
	printlnIntHex((int) p);
	println(p);
	*/

	va_list ap;
	char *s;
	int d;

	va_start(ap, fmt);

	while (*fmt) {
		switch (*fmt++) {
			case 's':
				s = va_arg(ap, char *);
				print("string: ");
				println(s);
				break;
			case 'd':
				d = va_arg(ap, int);
				print("int: ");
				printlnIntHex(d);
				break;
		}
	}

	va_end(ap);
}

void boot() {
	test("%s, %d, %d, %s", "abc", 333, 0x9876ABCD, "xxx-yyy");

	println("=========================");
	println("Hello, world!");
	print("aaa");
	print(" -- ");
	print("xxx");
	println("");

	println(num2Hex(0x1234, 4, false));
	println(num2Hex(0xabcd9876, 8, false));

	println(byte2Hex(0xF));
	println(short2Hex(0xaa55));
	println(int2Hex(0x11112222));

	printlnByteHex(0xF);
	printlnShortHex(0xaa55);
	printlnIntHex(0x11112222);
	println("=========================");
}

