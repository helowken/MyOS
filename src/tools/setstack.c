#include "common.h"

#define KB	1024

static char *progName;

static void setStackSize(size_t stackSize, char *path) {
	FILE *file;
	Elf32_Phdr phdr;
	off_t off;

	file = WFopen(path);
	findGNUStackHeader(path, file, &phdr, &off);
	phdr.p_memsz = stackSize;
	Fseek(path, file, off);
	Fwrite(path, file, &phdr, sizeof(phdr));
	Fclose(path, file);
}

static size_t getUnitSize(char c) {
	switch (c) {
		case 'm':
			return KB * KB;
		case 'k':
			return KB;
		case 'w':
			return 4;
		case 'b':
			return 1;
		default:
			fatal("Invalid unit: %c", c);
	}
}

static size_t parseSize(char *s) {
	char c;
	size_t size = 0;
	bool inited = false;

	while ((c = *s++)) {
		switch (c) {
			case 'm':
			case 'k':
			case 'w':
			case 'b':
				if (! inited)
				  fatal("Invalid size: %s", s);
				size *= getUnitSize(c);
				break;
			default:
				if (! between('0', c, '9'))
				  fatal("Invalid number: %c", c);
				size = size * 10 + (c - '0');
				inited = true;
				break;
		}
	}
	return size;
}

int main(int argc, char **argv) {
	size_t size;

	progName = argv[0];
	if (argc < 3) 
	  usageErr("%s stackSize program\n", progName);	

	size = parseSize(argv[1]);
	printf("Set stack size to: %d bytes\n", size);
	setStackSize(size, argv[2]);
	return EXIT_SUCCESS;
}
