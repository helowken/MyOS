#include "ctype.h"
#include "stdarg.h"
#include "string.h" 
#include "stdio.h"
#include "limits.h"
#include "time.h"
#include "unistd.h"
#include "minix/minlib.h"

static char *tb = "abcde";
static char *prog;

void testPrint() {
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
}

static void printTM(struct tm *tm) {
	printf("sec: %d, min: %d, hour: %d, mday: %d\n", 
			tm->tm_sec, tm->tm_min, tm->tm_hour, tm->tm_mday);
    printf("mon: %d, year: %d, wday: %d, yday: %d, isdst: %d\n",
			tm->tm_mon, tm->tm_year, tm->tm_wday, tm->tm_yday, tm->tm_isdst);
}

static char buf[50];

static void testStrftime(char *fmt, struct tm *tm) {
	strftime(buf, 50, fmt, tm);
	printf("%s: %s\n", fmt, buf);
}

void testTime() {
	time_t now;
	struct tm *tm;

	now = time(NULL);
	tm = gmtime(&now);
	printTM(tm);
	printf("ctime: %s", ctime(&now));
	printf("asctime: %s", asctime(tm));

	testStrftime("%c", tm);
	testStrftime("%x", tm);
	testStrftime("%X", tm);
	testStrftime("%A %B %I %p %j %m %y %U %w %W", tm);
}

void testRead() {
	int c;

	printf("=== before\n");
	if (read(STDIN_FILENO, &c, 1) < 0) 
	  fatal(prog, "read failed");
	printf("=== after\n");
	printf("=== c: %c\n", c);
}

int main(int argc, char **argv) {
	prog = argv[0];
	//testTime();
	//testPrint();
	testRead();
	return 0;
}
