#ifndef _UTIL_H
#define _UTIL_H

#include "stdint.h"
#include "string.h"

#define HEX_BUF_LEN	11	// '0x' + 4B (each byte 2 bits) + '\0'
#define BYTE_BITS	2
#define SHORT_BITS	4
#define INT_BITS	8

#define arraySize(a)		(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)		((a) + arraySize(a))
#define between(a, c, z)	((unsigned) ((c) - (a)) <= ((z) - (a)))


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
extern int detect88Mem();
extern int detectE820Mem();
extern int detectE801Mem(int *low, int *high, bool onStack);


bool numPrefix(char *s, char **ps);
bool numeric(char *s);
char *ul2a(u32_t n, unsigned b);
char *ul2a10(u32_t n);
long a2l(char *a);
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
void print88Mem();
void printE820Mem();
void printE801Mem();
void printRangeHex(char *pos, int num, int col);

#endif
