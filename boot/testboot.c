#include "code16gcc.h"

#define __NOINLINE  __attribute__((noinline))
#define __REGPARM   __attribute__ ((regparm(3)))
#define __NORETURN  __attribute__((noreturn))

extern void print(char *str);
extern void println(char *str);

void main2(char *str) {
	print(str);
	print(" *** ");
	println(str);
}

void main(){
	println("Hello, world!");
	println("aaabbbccc");

	main2("xyz-xyz");
}
