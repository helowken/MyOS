#ifndef _UTIL_H
#define _UTIL_H

#define HEX_BUF_LEN	11	// '0x' + 4B (each byte 2 bits) + '\0'
#define BYTE_BITS	2
#define SHORT_BITS	4
#define INT_BITS	8

typedef enum { false, true } bool;

extern int printf(char *fmt, ...);

void printSepLine();

// Below functions are for debug
extern void print(char *str);

extern void println(char *str);

char * 
num2Hex(int num, int len, bool withPrefix);

char * 
byte2Hex(char num);

char * 
short2Hex(short num);

char * 
int2Hex(int num);

void printNumHex(int num, int len);

void printlnNumHex(int num, int len);

void printByteHex(char num);

void printlnByteHex(char num);

void printShortHex(short num);

void printlnShortHex(short num);

void printIntHex(int num);

void printlnIntHex(int num);

#endif
