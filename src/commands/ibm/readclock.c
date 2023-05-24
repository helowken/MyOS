#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/ioc_cmos.h>

#define MAX_RETRIES		1
#define CMOS_DEV		"/dev/cmos"

static int nFlag = 0;	/* Tell what, but don't do it. */
static int y2kFlag = 0;	/* Interpret 1980 as 2000 for clock with Y2K bug. */

static void usageErr() { 
	fprintf(stderr, "Usage: settime [-n2]\n");
	exit(1);
}

static void errMsg(char *s) {
	static char *prompt = "readclock: ";
	
	fprintf(stderr, "%s%s\n", prompt, s);
	prompt = "";
}

int main(int argc, char **argv) {
	int fd;
	int i;
	int request;
	struct tm t1, t2, tmNow;
	time_t rtc;
	char date[64];

	/* Process options. */
	while (argc > 1) {
		char *p = *++argv;

		if (*p++ != '-')
		  usageErr();

		while (*p != 0) {
			switch (*p++) {
				case 'n': 
					nFlag = 1;
					break;
				case '2':
					y2kFlag = 1;
					break;
				default:
					usageErr();
			}
		}
		--argc;
	}

	/* Read the CMOS real time clock. */
	for (i = 0; i < MAX_RETRIES; ++i) {
		/* Sleep, unless first interation */
		if (i > 0) 
		  sleep(5);

		/* Open the CMOS device to read the system time. */
		if ((fd = open(CMOS_DEV, O_RDONLY)) < 0) {
			perror(CMOS_DEV);
			fprintf(stderr, "Couldn't open CMOS device.\n");
			exit(1);
		}
		request = y2kFlag ? CIOC_GET_TIME : CIOC_GET_TIME_Y2K;
		if (ioctl(fd, request, (void *) &t1) < 0) {
			perror("ioctl");
			fprintf(stderr, "Couldn't do CMOS ioctl.\n");
			exit(1);
		}
		close(fd);

		time(NULL);
		t1.tm_isdst = -1;		/* Do timezone calculations. */
		t2 = t1;

		rtc = mktime(&t1);		/* Transform to a time_t. */
		if (rtc != -1)
		  break;

		fprintf(stderr, "%s: Invalid time read from CMOS RTC: "
					"%d-%02d-%2d %02d:%02d:%02d\n",
					t2.tm_year + 1980, t2.tm_mon + 1, t2.tm_mday,
					t2.tm_hour, t2.tm_min, t2.tm_sec);
	}
	if (i >= MAX_RETRIES)
	  exit(1);

	/* Now set system time. */
	if (nFlag) {
		printf("stime(%lu)\n", (unsigned long) rtc);
	} else {
		if (stime(&rtc) < 0)  {
			errMsg("Not allowed to set time.");
			exit(1);
		}
	}
	tmNow = *localtime(&rtc);
	if (strftime(date, sizeof(date), 
			"%a %b %d %H:%M:%S %Z %Y", &tmNow) != 0) {
		if (date[8] == '0')
		  date[8] = ' ';
#if 0
		printf("%s [CMOS read via FS\n", date);
#endif
	}
	return 0;
}


