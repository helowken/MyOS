#include "common.h"
#include "util.h"


void getActivePartition(int deviceFd, PartitionEntry *pep) {
	int i, activeCount = 0;
	bool found = false;
	PartitionEntry pe;

	if (lseek(deviceFd, PART_TABLE_OFF, SEEK_SET) == -1)
	  errExit("lseek partition table");
	
	for (i = 0; i < NR_PARTITIONS; ++i) {
		if (read(deviceFd, &pe, sizeof(pe)) != sizeof(pe))
		  errExit("read partition %d from device", i);
		//printf("%d, %u, %u, %u\n", i, pe.status, pe.type, pe.lowSector);
		if (BOOTABLE(&pe)) {
			if (!found) {
				*pep = pe;
				found = true;
			}
			if (activeCount++ > 0)
			  fatal("more than 1 active partition");
		}
	}
	if (activeCount == 0)
	  fatal("no active partition found");
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
