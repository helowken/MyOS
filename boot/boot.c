#include "code16gcc.h"
#include "util.h"

void test(){
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

	printByteHex(0xF);
	printShortHex(0xaa55);
	printIntHex(0x11112222);
}
