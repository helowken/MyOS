#define nil	((void *) 0)
#include "sys/types.h"
#include "assert.h"
#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "limits.h"
#include "time.h"
#include "dirent.h"
#include "sys/stat.h"
#include "misc.h"
#include "tab.h"

#define isdigit(c)	((unsigned) ((c) - '0') < 10)

static char *getToken(char **ptr) {
/* Return a pointer to the next token in a string. Move *ptr to the end of
 * the token, and return a pointer to the start. If *ptr == start of token
 * then we're stuck against a newline or end of string.
 */
	char *start, *p;

	p = *ptr;
	while (*p == ' ' || *p == '\t') {
		++p;
	}

	start = p;
	while (*p != ' ' && *p != '\t' && *p != '\n' && *p != 0) {
		++p;
	}
	*ptr = p;
	return start;
}

static int rangeParse(char *file, char *data, bitmap_t map, int min,
			int max, int *wildcard) {
/* Parse a comma separated series of 'n', 'n-m' or 'n:m' numbers. 'n'
 * includes number 'n' in the bit map, 'n-m' includes all numbers between
 * 'n' and 'm' inclusive, and 'n:m' includes 'n+k*m' for any k if in range.
 * Numbers must fall between 'min' and 'max'. A '*' means all numbers. A
 * '?' is allowed as a synonym for the current minute, which only makes sense
 * in the minute filed, i.e. max must be 59. Return true iff parsed ok.
 */
	char *p;
	int end;
	int n, m;

	/* Clear all bits. */
	for (n = 0; n < 8; ++n) {
		map[n] = 0;
	}

	p = data;
	while (*p != ' ' && *p != '\t' && *p != '\n' && *p != 0) {
		++p;
	}
	end = *p;
	*p = 0;
	p = data;

	if (*p == 0) {
		log(LOG_ERR, "%s: not enough time fields\n", file);
	}

	/* Is it a '*'? */
	if (p[0] == '*' && p[1] == 0) {
		for (n = min; n <= max; ++n) {
			bitSet(map, n);
		}
		p[1] = end;
		*wildcard = 1;
		return 1;
	}
	*wildcard = 0;

	/* Parse a comma separated series of numbers or ranges. */
	for (;;) {
		if (*p == '?' && max == 59 && p[1] != '-') {
			n = localtime(&now)->tm_min;
			++p;
		} else {
			if (! isdigit(*p))
			  goto syntax;
			n = 0;
			do {
				n = 10 * n + (*p++ - '0');
				if (n > max)
				  goto range;
			} while (isdigit(*p));
		}
		if (n < min)
		  goto range;

		if (*p == '-') {	/* A range of the form 'n-m'? */
			++p;
			if (! isdigit(*p))
			  goto syntax;
			m = 0;
			do {
				m = 10 * m + (*p++ - '0');
				if (m > max)
				  goto range;
			} while (isdigit(*p));
			if (m < n)
			  goto range;
			do {
				bitSet(map, n);
			} while (++n <= m);
		} else if (*p == ':') {		/* A repest of the form 'n:m'? */
			++p;
			if (! isdigit(*p))
			  goto syntax;
			m = 0;
			do {
				m = 10 * m + (*p++ - '0');
				if (m > (max - min + 1))
				  goto range;
			} while (isdigit(*p));
			if (m == 0)
			  goto range;
			while (n >= min) {
				n -= m;
			}
			while ((n += m) <= max) {
				bitSet(map, n);
			}
		} else {	/* Simply a number */
			bitSet(map, n);
		}
		if (*p == 0)
		  break;
		if (*p++ != ',')	/* e.g, 2,3,4,5 */
		  goto syntax;
	}
	*p = end;
	return 1;

syntax:
	log(LOG_ERR, "%s: field '%s': bad syntax for a %d-%d time field\n",
				file, data, min, max);
	return 0;

range:
	log(LOG_ERR, "%s: field '%s': values out of the %d-%d allowed range\n",
				file, data, min, max);
	return 0;
}

void tabReschedule(CronJob *job) {
/* Reschedule one job. Compute the next time to run the job in job->runTime. */
	struct tm prevTm, nextTm, tmpTm;
	time_t noDstRunTime, dstRunTime;

	/* AT jobs are only run once. */
	if (job->atJob) {
		job->runTime = NEVER;
		return;
	}

	/* Was the job scheduled late last time? */
	if (job->late)
	  job->runTime = now;

	prevTm = *localtime(&job->runTime);
	prevTm.tm_sec = 0;
	
	nextTm = prevTm;
	nextTm.tm_min++;	/* Minimal increment */

	for (;;) {
		if (nextTm.tm_min > 59) {
			nextTm.tm_min = 0;
			nextTm.tm_hour++;
		}
		if (nextTm.tm_hour > 23) {
			nextTm.tm_min = 0;
			nextTm.tm_hour = 0;
			nextTm.tm_mday++;
		}
		if (nextTm.tm_mday > 31) {
			nextTm.tm_hour = nextTm.tm_min = 0;
			nextTm.tm_mday = 1;
			nextTm.tm_mon++;
		} 
		if (nextTm.tm_mon >= 12) {
			nextTm.tm_hour = nextTm.tm_min = 0;
			nextTm.tm_mday = 1;
			nextTm.tm_mon = 0;
			nextTm.tm_year++;
		}

		/* Verify tm_year. A crontab entry cannot restrict tm_year
		 * directly. However, certain dates (such as Feb, 29th) do
		 * not occur every year. We limit the difference between
		 * nextTm.tm_year and prevTm.tm_year to detect impossible dates
		 * (e.g, Feb, 31st). In theory every date occurs within a
		 * period of 4 years. However, some years at the end of a
		 * century are not leap years (e.g, the year 2100). An extra
		 * factor of 2 should be enough.
		 */
		if (nextTm.tm_year - prevTm.tm_year > 2 * 4) {
			job->runTime = NEVER;
			return;		/* Impossible combination */
		}

		if (! job->doWday) {
			/* Verify the mon and mday fields. If doWday and doMday
			 * are both true we have to merge both schedules. This
			 * is done after the call to mktime.
			 */
			if (! bitIsSet(job->mon, nextTm.tm_mon)) {
				nextTm.tm_mday = 1;
				nextTm.tm_hour = nextTm.tm_min = 0;
				nextTm.tm_mon++;
				continue;
			}

			/* Verify mday */
			if (! bitIsSet(job->mday, nextTm.tm_mday)) {
				/* Clear other fields */
				nextTm.tm_hour = nextTm.tm_min = 0;
				nextTm.tm_mday++;
				continue;
			}
		}

		/* Verify hour */
		if (! bitIsSet(job->hour, nextTm.tm_hour)) {
			/* Clear tm_min field */
			nextTm.tm_min = 0;
			nextTm.tm_hour++;
			continue;
		}
		
		/* Verify min */
		if (! bitIsSet(job->min, nextTm.tm_min)) {
			nextTm.tm_min++;
			continue;
		}

		/* Verify that we don't have any problem with DST. Try
		 * tm_isdst = 0 first. */
		tmpTm = nextTm;
		tmpTm.tm_isdst = 0;

		noDstRunTime = job->runTime = mktime(&tmpTm);
		if (job->runTime == -1) {
			/* This should not happen. */
			log(LOG_ERR,
			"mktime failed for %04d-%2d-%2d %02d:%02d:%02d\n",
				1900 + nextTm.tm_year, nextTm.tm_mon + 1,
				nextTm.tm_mday, nextTm.tm_hour,
				nextTm.tm_min, nextTm.tm_sec);
			job->runTime = NEVER;
			return;
		}

		tmpTm = *localtime(&job->runTime);
		if (tmpTm.tm_hour != nextTm.tm_hour ||
				tmpTm.tm_min != nextTm.tm_min) {
			assert(tmpTm.tm_isdst);
			tmpTm = nextTm;
			tmpTm.tm_isdst = 1;

			dstRunTime = job->runTime = mktime(&tmpTm);
			if (job->runTime == -1) {
				/* This should not happen. */
				log(LOG_ERR,
				"mktime failed for %04d-%2d-%2d %02d:%02d:%02d\n",
					1900 + nextTm.tm_year, nextTm.tm_mon + 1,
					nextTm.tm_mday, nextTm.tm_hour,
					nextTm.tm_min, nextTm.tm_sec);
				job->runTime = NEVER;
				return;
			}

			tmpTm = *localtime(&job->runTime);
			if (tmpTm.tm_hour != nextTm.tm_hour ||
					tmpTm.tm_min != nextTm.tm_min) {
				assert(! tmpTm.tm_isdst);
				/* We have a problem. This time neither exists 
				 * with DST nor without DST. Use the latest time, 
				 * which should be noDstRunTime.
				 */
				assert(noDstRunTime > dstRunTime);
				job->runTime = noDstRunTime;
			}
		}

		/* Verify this the combination (tm_year, tm_mon, tm_mday). */
		if (tmpTm.tm_mday != nextTm.tm_mday ||
				tmpTm.tm_mon != nextTm.tm_mon ||
				tmpTm.tm_year != nextTm.tm_year) {
			/* Wrong day */
			nextTm.tm_hour = nextTm.tm_min = 0;
			nextTm.tm_mday++;
			continue;
		}

		/* Match either weekday or month day if specified. */

		/* Check tm_wday */
		if (job->doWday && 
				bitIsSet(job->wday, tmpTm.tm_wday)) {
			/* OK, wday matched */
			break;
		}

		/* Check tm_mday */
		if (job->doMday && 
				bitIsSet(job->mon, tmpTm.tm_mon) &&
				bitIsSet(job->mday, tmpTm.tm_mday)) {
			/* OK, mon and mday matched */
			break;
		}

		if (! job->doWday && ! job->doMday) {
			/* No need to match wday and mday */
			break;
		}

		/* Wrong day */

		nextTm.tm_hour = nextTm.tm_min = 0;
		nextTm.tm_mday++;
	}

	/* Is job issuing lagging behind with the progress of time? */
	job->late = (job->runTime < now);

	/* The result is in job->runTime. */
	if (job->runTime < nextTime)
	  nextTime = job->runTime;
}

void tabFindATJob(char *atDir) {
/* Find the first to be executed AT job and kludge up an crontab job for it.
 * We set tab-file to "atdir/jobname", tab->data to "atdir/past/jobname",
 * and job->cmd to "jobname".
 */
	DIR *spoolDir;
	struct dirent *entry;
	time_t t0, t1;
	struct tm tmNow, tmT1;
	static char template[] = "YY.DDD.HHMM.UU";
	char firstJob[sizeof(template)];
	int i;
	Crontab *tab;
	CronJob *job;

	if ((spoolDir = opendir(atDir)) == nil)
	  return;

	tmNow = *localtime(&now);
	t0 = NEVER;

	while ((entry = readdir(spoolDir)) != nil) {
		/* Check if the name fits the template. */
		for (i = 0; template[i] != 0; ++i) {
			if (template[i] == '.') {
				if (entry->d_name[i] != '.')
				  break;
			} else {
				if (! isdigit(entry->d_name[i]))
				  break;
			}
		}
		if (template[i] != 0 || entry->d_name[i] != 0)
		  continue;

		/* Convert the name to a time. Careful with the century.
		 * Format: YY.DDD.HHMM.UU
		 */
		memset(&tmT1, 0, sizeof(tmT1));
		tmT1.tm_year = atoi(entry->d_name + 0);		/* YY */
		while (tmT1.tm_year < tmNow.tm_year - 10) {
			tmT1.tm_year += 100;
		}
		tmT1.tm_mday = 1 + atoi(entry->d_name + 3);	/* DDD */
		tmT1.tm_min = atoi(entry->d_name + 7);		/* HHMM */
		tmT1.tm_hour = tmT1.tm_min / 100;	/* HHMM = HH * 100 + MM */
		tmT1.tm_min %= 100;
		tmT1.tm_isdst = -1;
		if ((t1 = mktime(&tmT1)) == -1) {
			/* Illegal time? Try in winter time. */
			tmT1.tm_isdst = 0;
			if ((t1 = mktime(&tmT1)) == -1)
			  continue;
		}
		if (t1 < t0) {
			t0 = t1;
			strcpy(firstJob, entry->d_name);
		}
	}
	closedir(spoolDir);

	if (t0 == NEVER)	/* AT job spool is empty. */
	  return;

	/* Create new table and job structures. */
	tab = allocate(sizeof(*tab));
	tab->file = allocate((strlen(atDir) + 1 + sizeof(template)) * 
							sizeof(tab->file[0]));
	strcpy(tab->file, atDir);
	strcpy(tab->file, "/");
	strcpy(tab->file, firstJob);
	tab->data = allocate((strlen(atDir) + 6 + sizeof(template)) *
							sizeof(tab->data[0]));
	strcpy(tab->data, atDir);
	strcpy(tab->data, "/past/");
	strcpy(tab->data, firstJob);
	tab->user = nil;
	tab->mtime = 0;
	tab->current = 1;
	tab->next = crontabs;
	crontabs = tab;

	tab->jobs = job = allocate(sizeof(*job));
	job->next = nil;
	job->tab = tab;
	job->user = nil;
	job->cmd = tab->data + strlen(atDir) + 6;
	job->runTime = t0;
	job->late = 0;
	job->atJob = 1;		/* One AT job disguised as a cron job. */
	job->pid = IDLE_PID;

	if (job->runTime < nextTime)
	  nextTime = job->runTime;
}

void tabParse(char *file, char *user) {
/* Parse a crontab file and add its data to the tables. Handle errors by
 * yourself. Table is owned by 'user' if non-null.
 */
	Crontab **tabList, *tab;
	CronJob **jobList, *job;
	int fd;
	struct stat st;
	char *p, *q;
	size_t n;
	ssize_t r;
	int ok;
	int wc;		/* wildcard */

	for (tabList = &crontabs; (tab = *tabList) != nil; tabList = &tab->next) {
		if (strcmp(file, tab->file) == 0)
		  break;
	}

	/* Try to open the file. */
	if ((fd = open(file, O_RDONLY)) < 0 || fstat(fd, &st) < 0) {
		if (errno != ENOENT) 
		  log(LOG_ERR, "%s: %s\n", file, strerror(errno));
		if (fd != -1)
		  close(fd);
		return;
	}

	/* Forget it if the file is awfully big. */
	if (st.st_size > TAB_MAX) {
		log(LOG_ERR, "%s: %lu bytes is bigger than my %lu limit\n",
				file,
				(unsigned long) st.st_size,
				(unsigned long) TAB_MAX);
		return;
	}

	/* If the file is the same as before then don't bother. */
	if (tab != nil && st.st_mtime == tab->mtime) {
		close(fd);
		tab->current = 1;
		return;
	}

	/* Create a new table structure. */
	tab = allocate(sizeof(*tab));
	tab->file = allocate((strlen(file) + 1) * sizeof(tab->file[0]));
	strcpy(tab->file, file);
	tab->user = nil;
	if (user != nil) {
		tab->user = allocate((strlen(user) + 1) * sizeof(tab->user[0]));
		strcpy(tab->user, user);
	}
	tab->data = allocate((st.st_size + 1) * sizeof(tab->data[0]));
	tab->jobs = nil;
	tab->mtime = st.st_mtime;
	tab->current = 0;
	tab->next = *tabList;
	*tabList = tab;

	/* Pull a new table in core. */
	n = 0;
	while (n < st.st_size) {
		if ((r = read(fd, tab->data + n, st.st_size - n)) < 0) {
			log(LOG_CRIT, "%s: %s", file, strerror(errno));
			close(fd);
			return;
		}
		if (r == 0)
		  break;
		n += r;
	}
	close(fd);
	tab->data[n] = 0;
	if (strlen(tab->data) < n) {
		log(LOG_ERR, "%s contains a null character\n", file);
		return;
	}

	/* Parse the file. */
	jobList = &tab->jobs;
	p = tab->data;
	ok = 1;
	while (ok && *p != 0) {
		q = getToken(&p);
		if (*q == '#' || q == p) {
			/* Comment or empty. */
			while (*p != 0 && *p++ != '\n') {
			}
			continue;
		}

		/* One new job coming up. */
		*jobList = job = allocate(sizeof(*job));
		*(jobList = &job->next) = nil;
		job->tab = tab;

		if (! rangeParse(file, q, job->min, 0, 59, &wc)) {
			ok = 0;
			break;
		}

		q = getToken(&p);
		if (! rangeParse(file, q, job->hour, 0, 23, &wc)) {
			ok = 0;
			break;
		}

		q = getToken(&p);
		if (! rangeParse(file, q, job->mday, 1, 31, &wc)) {
			ok = 0;
			break;
		}
		job->doMday = ! wc;

		q = getToken(&p);
		if (! rangeParse(file, q, job->mon, 1, 12, &wc)) {
			ok = 0;
			break;
		}
		job->doMday |= ! wc;

		q = getToken(&p);
		if (! rangeParse(file, q, job->wday, 0, 7, &wc)) {
			ok = 0;
			break;
		}
		job->doWday = ! wc;

		/* 7 is Sunday, but 0 is common mistake because it is in the
		 * tm_wday range. We allow and even prefer it internally.
		 */
		if (bitIsSet(job->wday, 7)) {
			bitClear(job->wday, 7);
			bitSet(job->wday, 0);
		}

		/* The month range is 1-12, but tm_mon likes 0-11.
		 *
		 *	1 1111 1111 1110
		 * ->
		 *	0 1111 1111 1111
		 */
		job->mon[0] >>= 1;		
		if (bitIsSet(job->mon, 8))
		  bitSet(job->mon, 7);

		/* Scan for options. */
		job->user = nil;
		while (*(q = getToken(&p)) == '-') {
			++q;
			if (q[0] == '-' && q + 1 == p) {
				/* -- */
				q = getToken(&p);
				break;
			}
			while (q < p) {
				switch (*q++) {
					case 'u':
						if (q == p)
						  q = getToken(&p);
						if (q == p)
						  goto usage;
						memmove(q - 1, q, p - q);
						p[-1] = 0;
						job->user = q - 1;
						q = p;
						break;
					default:
usage:
						log(LOG_ERR, 
				"%s: bad options -%c, good options are: -u username\n",
								file, q[-1]);
						ok = 0;
						goto endTab;
				}
			}
		}

		/* A crontab owned by a user can only do things as that user. */
		if (tab->user != nil)
		  job->user = tab->user;

		/* Inspect the first character of the command. */
		job->cmd = q;
		if (q == p || *q == '#') {
			/* Rest of the line is empty, i.e. the commands are on
			 * the following lines indented by one tab.
			 */
			while (*p != 0 && *p++ != '\n') {
			}
			if (*p++ != '\t') {		/* A new line of job */
				log(LOG_ERR, "%s: contains an empty command\n", file);
				ok = 0;
				goto endTab;
			}
			while (*p != 0) {	/* Skip the whole command */
				if ((*q = *p++) == '\n') {
					if (*p != '\t')		/* A new line of job */
					  break;
					++p;
				}
				++q;
			}
		} else {
			/* The command is on this line. Alas we must now be
			 * backwards compatible and change %'s to newlines.
			 */
			p = q;
			while (*p != 0) {
				if ((*q = *p++) == '\n')	/* End of the command */
				  break;
				if (*q == '%')
				  *q = '\n';
				++q;
			}
		}
		*q = 0;		

		job->runTime = now;
		job->late = 0;			/* It is on time. */
		job->atJob = 0;			/* True cron job. */
		job->pid = IDLE_PID;	/* Not running yet. */
		tabReschedule(job);		/* Compute next time to run. */
	}
endTab:
	if (ok)
	  tab->current = 1;
}

void tabPurge(void) {
/* Remove table data that is no longer current. E.g. a crontab got removed.
 */
	Crontab **tabList, *tab;
	CronJob *job;

	tabList = &crontabs;
	while ((tab = *tabList) != nil) {
		if (tab->current) {
			/* Table is fine. */
			tab->current = 0;
			tabList = &tab->next;
		} else {
			/* Table is not, or no longer valid; delete. */
			*tabList = tab->next;
			while ((job = tab->jobs) != nil) {
				tab->jobs = job->next;
				deallocate(job);
			}
			deallocate(tab->data);
			deallocate(tab->file);
			deallocate(tab->user);
			deallocate(tab);
		}
	}
}

static CronJob *reapOrFind(pid_t pid) {
/* Find a finished job or search for the next one to run. */
	Crontab *tab;
	CronJob *job;
	CronJob *nextJob;

	nextJob = nil;
	nextTime = NEVER;
	for (tab = crontabs; tab != nil; tab = tab->next) {
		for (job = tab->jobs; job != nil; job = job->next) {
			if (job->pid == pid) {
				job->pid = IDLE_PID;
				tabReschedule(job);
			}
			if (job->pid != IDLE_PID)
			  continue;
			if (job->runTime < nextTime)
			  nextTime = job->runTime;
			if (job->runTime <= now)
			  nextJob = job;
		}
	}
	return nextJob;
}

void tabReapJob(pid_t pid) {
/* A job has finished. Try to find it among the crontab data and reschedule
 * it. Recompute time next to run a job.
 */
	reapOrFind(pid);
}

CronJob *tabNextJob(void) {
/* Find a job that should be run now. If none are found return null.
 * Update 'nextTime'.
 */
	return reapOrFind(NO_PID);
}

static void printMap(FILE *fp, bitmap_t map) {
	int lastBit = -1, bit;
	char *sep;

	sep = "";
	for (bit = 0; bit < 64; ++bit) {
		if (bitIsSet(map, bit)) {
			if (lastBit == -1)
			  lastBit = bit;
		} else {
			if (lastBit != -1) {
				fprintf(fp, "%s%d", sep, lastBit);
				if (lastBit != bit - 1) {
					fprintf(fp, "-%d", bit - 1);
				}
				lastBit = -1;
				sep = ",";
			}
		}
	}
}

void tabPrint(FILE *fp) {
/* Print out a stored crontab file for debugging purposes. */
	Crontab *tab;
	CronJob *job;
	char *p;
	struct tm tm;

	for (tab = crontabs; tab != nil; tab = tab->next) {
		fprintf(fp, "tab->file = \"%s\"\n", tab->file);
		fprintf(fp, "tab->user = \"%s\"\n", 
					tab->user == nil ? "(root)" : tab->user);
		fprintf(fp, "tab->mtime = %s", ctime(&tab->mtime));

		for (job = tab->jobs; job != nil; job = job->next) {
			if (job->atJob) {
				fprintf(fp, "AT job");
			} else {
				printMap(fp, job->min);
				fputc(' ', fp);
				printMap(fp, job->hour);
				fputc(' ', fp);
				printMap(fp, job->mday);
				fputc(' ', fp);
				printMap(fp, job->mon);
				fputc(' ', fp);
				printMap(fp, job->wday);
			}
			if (job->user != nil && job->user != tab->user) {
				fprintf(fp, " -u %s", job->user);	
			}
			fprintf(fp, "\n\t");
			for (p = job->cmd; *p != 0; ++p) {
				fputc(*p, fp);
				if (*p == '\n')
				  fputc('\t', fp);
			}
			fputc('\n', fp);
			tm = *localtime(&job->runTime);
			fprintf(fp, "    rtime = %.24s%s\n", asctime(&tm),
						tm.tm_isdst ? " (DST)" : "");
			if (job->pid != IDLE_PID)
			  fprintf(fp, "    pid = %ld\n", (long) job->pid);
		}
	}
}

