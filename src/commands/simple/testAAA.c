#include "ctype.h"
#include "stdarg.h"
#include "string.h" 
#include "stdio.h"
#include "limits.h"

static char *tb = "abcde";

int main(int argc, char **argv) {
	printf("1=== %s, 3_h=%hi, 4_i=%i, 5000000_l=%li, p=%p\n", "abc", USHRT_MAX + 2, 4, 5000000, tb);
	printf("2=== 6_b=%b, 7_o=%o, 8_u=%u, 128_x=%x, 129_X=%X\n", 6, 7, 8, 255, 254);
	printf("3=== 7=%#o, 71=%#x, 81=%#X\n", 0747, 0xAF4, 0xbe1);

	short s;
	int i;
	long l;
	printf("4===%hn %n %3ln", &s, &i, &l);
	printf("1=%hd, 2=%d, 3=%li\n", s, i, l);

	printf("5===%+d, %+d\n", -333, 444);
	printf("5===% d, % d\n", -333, 444);
	printf("5===%06d, %06d, %06o, %06x\n", -333, 444, 0555, 0xABC);
	printf("6===%-8d, %8d, %*d,\n", 666, 777, -8, 888);
	printf("7===%.4s\n", "aaaaaa");
	return 0;
}
