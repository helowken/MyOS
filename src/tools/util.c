#include "common.h"
#include "util.h"

void getActivePartition(char *device, int fd, PartitionEntry *pep) {
	PartitionEntry table[NR_PARTITIONS];

	getPartitionTable(device, fd, PART_TABLE_OFF, table);
	findActivePartition(table, pep);	
}

void findActivePartition(PartitionEntry *table, PartitionEntry *pep) {
	int i, activeCount = 0;
	bool found = false;

	for (i = 0; i < NR_PARTITIONS; ++i) {
		if (BOOTABLE(&table[i])) {
			if (!found) {
				*pep = table[i];
				found = true;
			}
			if (activeCount++ > 0)
			  fatal("more than 1 active partition");
		}
	}
	if (activeCount == 0)
	  fatal("no active partition found");
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
	PartitionEntry pe, table[NR_PARTITIONS];
	uint32_t lowSector;

	s = strchr(device, ':');
	if (s == NULL) {
		fd = ROpen(device);
		getActivePartition(device, fd, &pe);
		if (bootSecPtr) 
		  *bootSecPtr = 0;
		lowSector = pe.lowSector;
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

int ROpen(char *fileName) {
	return Open(fileName, O_RDONLY);
}

int WOpen(char *fileName) {
	return Open(fileName, O_WRONLY);
}

int RWOpen(char *fileName) {
	return Open(fileName, O_RDWR);
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

FILE *Fopen(char *fileName) {
	FILE *file;
	struct stat st;

	if (stat(fileName, &st) < 0)
	  errExit("stat \"%s\"", fileName);
	if (!S_ISREG(st.st_mode))
	  errExit("\"%s\" is not a fileName.", fileName);
	if ((file = fopen(fileName, "r")) == NULL)
	  errExit("fopen \"%s\"", fileName);
	return file;
}

void Fseek(char *fileName, FILE *file, long off) {
	if (fseek(file, off, SEEK_SET) == -1)
	  errExit("fseek %s", fileName);
}

void Fread(char *fileName, FILE *file, void *buf, size_t size) {
	Fread2(fileName, file, buf, size, 1);
}

void Fread2(char *fileName, FILE *file, void *buf, size_t size, size_t num) {
	if (fread(buf, size, num, file) != num || ferror(file))
	  errExit("fread %s", fileName);
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
