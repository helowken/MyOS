#define __NOINLINE  __attribute__((noinline))
#define __REGPARM   __attribute__ ((regparm(3)))
#define __NORETURN  __attribute__((noreturn))

#define HEX_BUF_LEN	11	// '0x' + 4B (each byte 2 bits) + '\0'
#define BYTE_BITS	2
#define SHORT_BITS	4
#define INT_BITS	8

typedef enum { false, true } bool;


extern void print(char *str);
extern void println(char *str);


char* num2Hex(int num, int len, bool withPrefix);

void printNumHex(int num, int len);

char* byte2Hex(char num);

char* short2Hex(short num);

char* int2Hex(int num);

void printByteHex(char num);

void printShortHex(short num);

void printIntHex(int num);


