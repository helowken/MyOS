#include "code16gcc.h"
#include "util.h"

extern int detectLowMem();

void printLowMem() {
	printShortHex(detectLowMem());
	println("(KB)");
}
