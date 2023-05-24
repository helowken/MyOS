#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>

#define START_DAY	0	/* See ctime(3) */
#define LEAP_DAY	START_DAY + 59
#define MAX_DAY_NR	START_DAY + 365
#define NO_DAY		-2
static char CRON_PID[] = "/usr/run/cron.pid";

static int getLtime(char *t) {
	/* hour is in [00 ~ 23]
	 * minute is in [00 ~ 59]
	 */
	if (t[4] == '\0' && t[3] >= '0' && t[3] <= '9' &&
		t[2] >= '0' && t[2] <= '5' && t[1] >= '0' && t[1] <= '9' &&
		(t[0] == '0' || t[0] == '1' || (t[1] <= '3' && t[0] == '2')))
	  return atoi(t);
	else 
	  return -1;
}

static int digitString(char *s) {
	while (*s >= '0' && *s <= '9') {
		++s;
	}
	return *s == '\0' ? 1 : 0;
}

static int getLday(char *m, char *d) {
	int i, day, im;
	static int cumDay[] = {0, 0, 31, 60, 91, 121, 152,
				182, 213, 244, 274, 305, 335};
	static struct date {
		char * mon;
		int dayCnt;
	} *pc, kal[] = {
		{ "Jan", 31 }, { "Feb", 29 }, { "Mar", 31 }, { "Apr", 30 },
		{ "May", 31 }, { "Jun", 30 }, { "Jul", 31 }, { "Aug", 31 },
		{ "Sep", 30 }, { "Oct", 31 }, { "Nov", 30 }, { "Dec", 31 }
	};

	pc = kal;
	im = (digitString(m) ? atoi(m) : 0);
	m[0] &= 0337;	/* Convert lowercase to uppercase */
	for (i = 1; i < 13 && strcmp(m, pc->mon) && im != i; ++i, ++pc) {
	}
	if (i < 13 && 
			(day = (digitString(d) ? atoi(d) : 0)) && 
			day <= pc->dayCnt) {
		if (! START_DAY)	/* The first day of year is 0. */
		  --day;
		return day + cumDay[i];
	}
	return -1;
}

/* The name of the file created in /usr/spool/at by at
 * is YY.DDD.HHMM.UU (where YY, DDD, HH, and MM give the 
 * time to execute and UU is a unique number).
 *
 * Usage: at time [month day] [file]
 */
int main(int argc, char **argv, char **envp) {
	int i, c, mask, ltime, year, lday = NO_DAY, currTimeNum, currDayNum;
	char buf[64], job[30], pastJob[35], *dp, *sp;
	struct tm *tm;
	long clock;
	FILE *fp;
	char pwd[PATH_MAX + 1];

/*----------------------------------------------------------------------*
 *  Check arguments & pipe to "pwd"  *
 *----------------------------------------------------------------------*/
	if (argc < 2 || argc > 5) {
		fprintf(stderr, "Usage: %s time [month day] [file]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if ((ltime = getLtime(argv[1])) == -1) {
		fprintf(stderr, "%s: wrong time specification\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if ((argc == 4 || argc == 5) && 
			(lday = getLday(argv[2], argv[3])) == -1) {
		fprintf(stderr, "%s: wrong date specification\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if ((argc == 3 || argc == 5) &&
			open(argv[argc - 1], O_RDONLY) == -1) {
		fprintf(stderr, "%s: cannot find: %s\n", argv[0], argv[argc - 1]);
		exit(EXIT_FAILURE);
	}
	if (getcwd(pwd, sizeof(pwd)) == NULL) {
		fprintf(stderr, "%s: cannot determine current directory: %s\n");
		exit(EXIT_FAILURE);
	}

/*----------------------------------------------------------------------*
 *  Determine execution time and create 'at'job file  *
 *----------------------------------------------------------------------*/
	time(&clock);
	tm = localtime(&clock);
	year = tm->tm_year;
	currTimeNum = tm->tm_hour * 100 + tm->tm_min;
	if (lday == NO_DAY) {	/* No [month day] given */
		lday = tm->tm_yday;
		/* e.g., "14:30" -> 14 * 100 + 30 = 1430 */
		if (ltime <= currTimeNum) {	
			/* Expired, execute at the next day */
			++lday;
			if ((lday == MAX_DAY_NR && (year % 4)) ||	
					lday == MAX_DAY_NR + 1) {
				lday = START_DAY;
				++year;
			}
		}
	} else {
		/* lday is the day of a leap year */
		switch (year % 4) {
			case 0:
				if (lday < tm->tm_yday ||
						(lday == tm->tm_yday && ltime <= currTimeNum)) {
					/* Expired, execute at the next year */
					++year;
					if (lday > LEAP_DAY) {
						/* The next year is not a leap year,
						 * so lday needs -1.
						 */ 
						--lday;
					}
				}
				break;
			case 1:
			case 2:
				if (lday > LEAP_DAY) {
					/* This year is not a leap year,
					 * so lday needs -1.
					 */
					--lday;
				}
				if (lday < tm->tm_yday || 
						(lday == tm->tm_yday && ltime <= currTimeNum)) {
					/* Expired, execute at the next year,
					 * since next year is also not a leap year,
					 * lday does not need +1 again.
					 */
					++year;
				}
				break;
			case 3:
				currDayNum = (lday > LEAP_DAY ? tm->tm_yday + 1 : tm->tm_yday);
				if (lday < currDayNum ||
						(lday == currDayNum && ltime <= currTimeNum)) {
					/* Expired, execute at the next year,
					 * since the next year is a leap year,
					 * lday does not need to be changed.
					 */
					++year;
				} else if (lday > LEAP_DAY) {
					/* Not expire, since this year is not a leap year,
					 * lday needs -1.
					 */
					--lday;
				}
				break;
		}
	}
	sprintf(job, "/usr/spool/at/%02d.%03d.%04d.%02d",
		year % 100, lday, ltime, getpid() % 100);
	sprintf(pastJob, "usr/spool/at/past/%02d.%03d.%04d.%02d",
		year % 100, lday, ltime, getpid() % 100);
	mask = umask(0077);
	if ((fp = fopen(pastJob, "w")) == NULL) {
		fprintf(stderr, "%s: cannot create %s: %s\n",
			argv[0], pastJob, strerror(errno));
		exit(EXIT_FAILURE);
	}

/*----------------------------------------------------------------------*
 *  Write environment and command(s) to 'at'job file  *
 *----------------------------------------------------------------------*/
	i = 0;
	while ((sp = envp[i++]) != NULL) {
		dp = buf;
		while ((c = *sp++) != '\0' && 
				c != '=' && 
				dp < buf + sizeof(buf) - 1) {
			*dp++ = c;
		}
		if (c != '=')
		  continue;
		*dp = '\0';
		fprintf(fp, "%s='", buf);
		while (*sp != 0) {
			if (*sp == '\'') {
			/* The first ' closes the opening single quote, then \' escapes 
			 * a single quote character, and the final ' reopens the single 
			 * quote.
			 */
				fprintf(fp, "'\\''");
			} else
			  fputc(*sp, fp);
			++sp;
			
		}
		fprintf(fp, "'; export %s\n", buf);
	}
	fprintf(fp, "cd '%s'\n", pwd);
	fprintf(fp, "umask %o\n", mask);
	if (argc == 3 || argc == 5) 
	  fprintf(fp, "%s\n", argv[argc - 1]);
	else {	/* Read from stdinput */
		while ((c = getchar()) != EOF) {
			putc(c, fp);
		}
	}
	fclose(fp);

	if (chown(pastJob, getuid(), getgid()) == -1) {
		fprintf(stderr, "%s: cannot set ownership of %s: %s\n",
					argv[0], pastJob, strerror(errno));
		unlink(pastJob);
		exit(EXIT_FAILURE);
	}
	/* "Arm" the job. */
	if (rename(pastJob, job) == -1) {
		fprintf(stderr, "%s: cannot move %s to %s: %s\n",
					argv[0], pastJob, job, strerror(errno));
		unlink(pastJob);
		exit(EXIT_FAILURE);
	}
	printf("%s: %s created\n", argv[0], job);

	/* Alert cron to the new situation. */
	if ((fp = fopen(CRON_PID, "r")) != NULL) {
		unsigned long pid;

		pid = 0;
		while ((c = fgetc(fp)) != EOF && c != '\n') {
			if ((unsigned) (c - '0') >= 10) {
				pid = 0;
				break;
			}
			pid = 10 * pid + (c - '0');
			if (pid >= 30000) {
				pid = 0;
				break;
			}
		}
		if (pid > 1)
		  kill((pid_t) pid, SIGHUP);
	}
	return EXIT_SUCCESS;
}




