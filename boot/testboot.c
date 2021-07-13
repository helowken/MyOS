#include "code16gcc.h"
asm ("jmpl $0x0, $main\n\t");

#define __NOINLINE  __attribute__((noinline))
#define __REGPARM   __attribute__ ((regparm(3)))
#define __NORETURN  __attribute__((noreturn))

void __NOINLINE __REGPARM print(char *s) {
	char *c = s;
	//char *c = "ABCD";
	while (*c) {
		asm volatile (
			"int $0x10" 
			: 
			: "a"(0x0e00 | *c)
		);
		c++;
	}
}

void __NORETURN main(){
	//print("Hello22, world!\n");
	
	//test('y');
	asm volatile (
		"call test"
		:
		: "b"('y')
	);
	while(1);
}


