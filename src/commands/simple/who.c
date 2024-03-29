#define nil	0
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <utmp.h>
#include <time.h>
#include <string.h>
#include <minix/paths.h>

static char PATH_UTMP[] = _PATH_UTMP;
static char day[] = "SunMonTueWedThuFriSat";
static char month[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

void main(int argc, char **argv) {
	char *tmp = PATH_UTMP;
	FILE *f;
	struct utmp ut;
	struct tm *tm;
	int slot, wtmp = 0, once = 0;

	if (argc > 3) {
		fprintf(stderr, "Usage: who <account-file>  |  who am i\n");
		exit(EXIT_FAILURE);
	}
	if (argc == 2) {
		tmp = argv[1];
		wtmp = 1;
	}

	if ((f = fopen(tmp, "r")) == nil) {
		fprintf(stderr, "who: can't open %s\n", tmp);
		exit(EXIT_FAILURE);
	}
	if (argc == 3) {
		if ((slot = ttyslot()) < 0) {
			fprintf(stderr, "who: no access to terminal.\n");
			exit(EXIT_FAILURE);
		}
		fseek(f, (off_t) sizeof(ut) * slot, 0);
		once = 1;
	}

	while (fread((char *) &ut, sizeof(ut), 1, f) == 1) {
		if (! wtmp && ut.ut_user[0] == 0)
		  continue;

		tm = localtime(&ut.ut_time);

		printf("%-9.8s %-9.8s %.3s %.3s %2d %02d:%02d",
				ut.ut_user,
				ut.ut_line,
				day + (3 * tm->tm_wday),
				month + (3 * tm->tm_mon),
				tm->tm_mday,
				tm->tm_hour,
				tm->tm_min
		);

		if (ut.ut_host[0] != 0)
		  printf("  (%.*s)", (int) sizeof(ut.ut_host), ut.ut_host);

		printf("\n");
		if (once)
		  break;
	}
}
