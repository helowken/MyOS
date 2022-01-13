#ifndef UTIL_H
#define UTIL_H

#define PARAM_SEC_OFF		1			/* next to the bootblock */
#define SECTOR_SIZE			512
#define SECTORS(n)			(((n) + ((SECTOR_SIZE) - 1)) / (SECTOR_SIZE))
#define OFFSET(n)			((n) * (SECTOR_SIZE))

void getActivePartition(int deviceFd, PartitionEntry *pe);

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
