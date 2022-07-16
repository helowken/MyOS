#ifndef UTIL_H
#define UTIL_H

#define SECTOR_SIZE			512
#define SECTORS(n)			(((n) + ((SECTOR_SIZE) - 1)) / (SECTOR_SIZE))
#define OFFSET(n)			((n) * (SECTOR_SIZE))
#define RATIO(n)			((n) / SECTOR_SIZE)

#define arraySize(a)		(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)		((a) + arraySize(a))
#define between(a, c, z)	((unsigned) ((c) - (a)) <= ((z) - (a)))

void getActivePartition(int deviceFd, PartitionEntry *pe);

void getActivePartition2(int deviceFd, PartitionEntry *pep, unsigned long *totalSectors);

int Open(char *fileName, int flags);

int ROpen(char *fileName);

int WOpen(char *fileName);

int RWOpen(char *fileName);

off_t getFileSize(char *pathName);

ssize_t Read(char *fileName, int fd, char *buf, size_t len);

void Write(char *fileName, int fd, char *buf, size_t len);

void Lseek(char *fileName, int fd, off_t offset);

void Close(char *fileName, int fd);

#endif
