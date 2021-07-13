#include "code16gcc.h"
asm ("jmpl $0x0, $main\n\t");

#define __NOINLINE  __attribute__((noinline))
#define __REGPARM   __attribute__ ((regparm(3)))
#define __NORETURN  __attribute__((noreturn))

/* BIOS interrupts must be done with inline assembly */
void __NOINLINE __REGPARM print(char *s) {
	char *c = s;
	while (*c) {
		asm volatile (
			"int $0x10" 
			: 
			: "a"(0x0e00 | *c)
		);
		c++;
	}
}
/* and for everything else you can use C! Be it traversing the filesystem, or verifying the kernel image etc.*/

void __NORETURN main(){
	print("Hello22, world!\n");
	while(1);
}



