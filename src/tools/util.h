#ifndef UTIL_H
#define UTIL_H

#include "image.h"

#define SECTOR_SIZE			512
#define RESERVED_SECTORS	2048
#define SECTORS(n)			(((n) + SECTOR_SIZE - 1) / SECTOR_SIZE)
#define OFFSET(n)			((n) * SECTOR_SIZE)
#define RATIO(n)			((n) / SECTOR_SIZE)

#define arraySize(a)		(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)		((a) + arraySize(a))
#define between(a, c, z)	((unsigned) ((c) - (a)) <= ((z) - (a)))

char *getProgName(char **argv);

int getPartIdx(char *s);

void getPartitionTable(char *device, int fd, off_t off, PartitionEntry *table);

char *parseDevice(char *device, uint32_t *bootSecPtr, PartitionEntry *pe);

#define ROpen(fileName)		Open(fileName, O_RDONLY)
#define WOpen(fileName)		Open(fileName, O_WRONLY)
#define RWOpen(fileName)	Open(fileName, O_RDWR)
int Open(char *fileName, int flags);

off_t getFileSize(char *pathName);

ssize_t Read(char *fileName, int fd, char *buf, size_t len);

void Write(char *fileName, int fd, char *buf, size_t len);

void Lseek(char *fileName, int fd, off_t offset);

void Close(char *fileName, int fd);

void CopyTo(char *fileName, int fd, int dstFd);

#define RFopen(file)		Fopen(file, "r")
#define WFopen(file)		Fopen(file, "r+")
FILE *Fopen(char *file, const char *mode);

void Fseek(char *fileName, FILE *file, long off);

void Fread2(char *fileName, FILE *file, void *buf, size_t size, size_t num);
#define Fread(fileName, file, buf, size)	Fread2(fileName, file, buf, size, 1)

void Fwrite2(char *fileName, FILE *file, void *buf, size_t size, size_t num);
#define Fwrite(fileName, file, buf, size)	Fwrite2(fileName, file, buf, size, 1)

void Fclose(char *fileName, FILE *file);

void *Malloc(size_t size);

void checkElfHeader(char *fileName, Elf32_Ehdr *ehdrPtr);

#define findGNUStackHeader(fileName, file, phdrPtr, off)	\
	findGNUStackHeader2(fileName, file, NULL, phdrPtr, off)
void findGNUStackHeader2(char *fileName, FILE *file, Elf32_Ehdr *ehdr,
			Elf32_Phdr *phdrPtr, off_t *off);
#endif
