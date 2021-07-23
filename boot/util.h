#ifndef _UTIL_H
#define _UTIL_H

#include "stdint.h"
#include "string.h"

#define HEX_BUF_LEN	11	// '0x' + 4B (each byte 2 bits) + '\0'
#define BYTE_BITS	2
#define SHORT_BITS	4
#define INT_BITS	8

typedef enum { false, true } bool;

typedef struct {
	uint64_t base;
	uint64_t len;
	uint32_t type;
	uint32_t exAttrs;
} E820MemEntry;


extern int printf(char *fmt, ...);
extern void print(char *str);
extern void println(char *str);
extern int detectLowMem();
extern int detectE820Mem();


char* num2Hex(int num, int len, bool withPrefix);
char* byte2Hex(char num);
char* short2Hex(short num);
char* int2Hex(int num);
void printSepLine();
void printNumHex(int num, int len);
void printlnNumHex(int num, int len);
void printByteHex(char num);
void printlnByteHex(char num);
void printShortHex(short num);
void printlnShortHex(short num);
void printIntHex(int num);
void printlnIntHex(int num);
void printLowMem();
void printE820Mem();


#endif
