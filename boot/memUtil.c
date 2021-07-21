#include "code.h"
#include "util.h"
#include "stdint.h"
#include "string.h"

extern int detectLowMem();

extern int detectE820Mem();

const char *destE820 = (char *) 0x8000;

typedef struct {
	uint64_t base;
	uint64_t len;
	uint32_t type;
	uint32_t exAttrs;
} E820MemEntry;

void printLowMem() {
	printf("Low Memory: %d KB\n", detectLowMem());
}

void printE820Mem() {
	int i, count;

	count = detectE820Mem();
	printf("E820 Memory Entries: %d\n\n", count);

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

