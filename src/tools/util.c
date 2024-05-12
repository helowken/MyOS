#include "common.h"
#include "image.h"
#include "util.h"

char *getProgName(char **argv) {
	char *progName;

	if ((progName = strrchr(argv[0], '/')) == NULL)
	  progName = argv[0];
	else
	  ++progName;

	return progName;
}

int getPartIdx(char *v) {
	char *s;
	int partIdx;

	partIdx = strtol(v, &s, 10);
	if (*s != '\0' || partIdx < 0 || partIdx > 3)
	  fatal("invalid partition index: %s", v);

	return partIdx;
}

void getPartitionTable(char *device, int fd, off_t off, PartitionEntry *table) {
	int i;

	Lseek(device, fd, off);
	for (i = 0; i < NR_PARTITIONS; ++i) {
		Read(device, fd, (char *) &table[i], sizeof(PartitionEntry));
	}
}

char *parseDevice(char *device, uint32_t *bootSecPtr, PartitionEntry *pep) {
	int fd;
	char *s, *path;
	int len, part, subPart;
	uint32_t lowSector;
	PartitionEntry table[NR_PARTITIONS];

	s = strchr(device, ':');
	if (s == NULL) {
		if (bootSecPtr) 
		  *bootSecPtr = 0;
	} else {
		path = device;
		device = s + 1;
		*s = 0;
		fd = ROpen(device);

		len = strlen(path);
		if (len == 2) {
			sscanf(path, "p%d", &part);
			subPart = -1;
		} else if (len == 4) {
			sscanf(path, "p%ds%d", &part, &subPart);
		}

		if (!between(0, part, NR_PARTITIONS))
		  fatal("Invalid part: %d\n", part);
		getPartitionTable(device, fd, PART_TABLE_OFF, table);
		if (table[part].type == NO_PART)
		  fatal("Invalid part: %d\n", part);

		lowSector = table[part].lowSector;
		if (bootSecPtr)
		  *bootSecPtr = lowSector;

		if (subPart != -1) {
			if (!between(0, subPart, NR_PARTITIONS))
			  fatal("Invalid sub-part: %d\n", subPart);
			getPartitionTable(device, fd, 
					OFFSET(lowSector) + PART_TABLE_OFF, table);
			if (table[subPart].type == NO_PART)
			  fatal("Invalid sub-part: %d\n", subPart);

			lowSector = table[subPart].lowSector;
			if (pep)
			  *pep = table[subPart];
		} else if (pep) {
			*pep = table[part];
		}
	}
	return device;
}

int Open(char *fileName, int flags) {
	int fd;

	if ((fd = open(fileName, flags)) == -1)
	  errExit("open %s", fileName);
	return fd;
}

off_t getFileSize(char *pathName) {
	struct stat sb;
	if (stat(pathName, &sb) == -1)
	  errExit("stat");
	return sb.st_size;
}

ssize_t Read(char *fileName, int fd, char *buf, size_t len) {
	ssize_t count;

	if ((count = read(fd, buf, len)) == -1)
	  errExit("read %s", fileName);
	return count;
}

void Write(char *fileName, int fd, char *buf, size_t len) {
	if (write(fd, buf, len) != len)
	  errExit("write %s", fileName);
}

void Lseek(char *fileName, int fd, off_t offset) {
	if (lseek(fd, offset, SEEK_SET) == -1)
	  errExit("lseek %s", fileName);
}

void Close(char *fileName, int fd) {
	if (close(fd) == -1)
	  errExit("close %s", fileName);
}

FILE *Fopen(char *fileName, const char *mode) {
	FILE *file;
	struct stat st;

	if (stat(fileName, &st) < 0)
	  errExit("stat \"%s\"", fileName);
	if (!S_ISREG(st.st_mode))
	  errExit("\"%s\" is not a fileName.", fileName);
	if ((file = fopen(fileName, mode)) == NULL)
	  errExit("fopen \"%s\"", fileName);
	return file;
}

void Fseek(char *fileName, FILE *file, long off) {
	if (fseek(file, off, SEEK_SET) == -1)
	  errExit("fseek %s", fileName);
}

void Fread2(char *fileName, FILE *file, void *buf, size_t size, size_t num) {
	if (fread(buf, size, num, file) != num || ferror(file))
	  errExit("fread %s", fileName);
}

void Fwrite2(char *fileName, FILE *file, void *buf, size_t size, size_t num) {
	if (fwrite(buf, size, num, file) != num || ferror(file))
	  errExit("fwrite %s", fileName);
}

void Fclose(char *fileName, FILE *file) {
	if (fclose(file) != 0) 
	  errExit("fclose %s", fileName);
}

void *Malloc(size_t size) {
	void *ptr;
	if (! (ptr = malloc(size)))
	  fatal("couldn't allocate %d", size);
	return ptr;
}

void checkElfHeader(char *fileName, Elf32_Ehdr *ehdrPtr) {
	if (ehdrPtr->e_ident[EI_MAG0] != ELFMAG0 ||
				ehdrPtr->e_ident[EI_MAG1] != ELFMAG1 ||
				ehdrPtr->e_ident[EI_MAG2] != ELFMAG2 ||
				ehdrPtr->e_ident[EI_MAG3] != ELFMAG3)
	  fatal("%s is not an ELF file.\n", fileName);
	
	if (ehdrPtr->e_ident[EI_CLASS] != ELFCLASS32)
	  fatal("%s is not an 32-bit executable.\n", fileName);

	if (ehdrPtr->e_ident[EI_DATA] != ELFDATA2LSB)
	  fatal("%s is not little endian.\n", fileName);

	if (ehdrPtr->e_ident[EI_VERSION] != EV_CURRENT)
	  fatal("%s has an invalid version.\n", fileName);

	if (ehdrPtr->e_type != ET_EXEC)
	  fatal("%s is not an executable.\n", fileName);
}

void findGNUStackHeader2(char *path, FILE *file, Elf32_Ehdr *ehdr,
			Elf32_Phdr *phdr, off_t *off) {
	Elf32_Ehdr tmpEhdr;
	int i;

	if (ehdr == NULL) {
		ehdr = &tmpEhdr;
		Fread(path, file, ehdr, sizeof(*ehdr));
		checkElfHeader(path, ehdr);
	}
	Fseek(path, file, ehdr->e_phoff);
	*off = ehdr->e_phoff;

	for (i = 0; i < ehdr->e_phnum; ++i) {
		Fread(path, file, phdr, sizeof(*phdr));
		if (phdr->p_type == PT_GNU_STACK) 
		  return;
		*off += sizeof(*phdr);
	}
	fatal("No GNU_STACK program header found.");
}

void CopyTo(char *fileName, int fd, int dstFd) {
	char buf[BUFSIZ];
	ssize_t off;

	while ((off = read(fd, buf, BUFSIZ)) > 0) {
		if (write(dstFd, buf, off) != off) 
		  errExit("write");
	}
	if (off == -1)
	  errExit("read %s", fileName);
}

