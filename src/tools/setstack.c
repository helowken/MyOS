#include "common.h"

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

int main(int argc, char **argv) {
	progName = argv[0];
	if (argc < 3) 
	  usageErr("%s stackSize program\n", progName);	

	setStackSize(atoi(argv[1]), argv[2]);
	return EXIT_SUCCESS;
}
