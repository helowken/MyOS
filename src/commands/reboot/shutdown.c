#define _POSIX_SOURCE	1
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <utmp.h>
#include <errno.h>
#include <minix/minlib.h>

static char SHUT_PID[] = "/usr/run/shutdown.pid";
static char NO_LOGIN[] = "/etc/nologin";	/* see login.c */

static long waitTime = 0L;
static char message[1024];
static char info[80];
static char rebootFlag = 'h';	/* Default is halt */
static char *rebootCode = "";	/* Optional monitor code */
static int infoMin, infoHour;	
char *prog;

extern char *itoa(int n);
extern void wall(char *when, char *extra);

static void usageErr() {
	fputs("Usage: shutdown [-hrRmk] [-x code] [time [message]]\n", stderr);
	fputs("       -h -> halt system after shutdown\n", stderr);
	fputs("       -r -> reboot system after shutdown\n", stderr);
	fputs("       -R -> reset system after shutdown\n", stderr);
	fputs("       -x -> return to the monitor doing...\n", stderr);
	fputs("       -m -> read a shutdown message from standard input\n", stderr);
	fputs("       -k -> stop an already running shutdown\n", stderr);
	fputs("       code -> boot monitor code to be executed\n", stderr);
	fputs("       time -> keyword ``now'', minutes before shutdown ``+5'',\n", stderr);
	fputs("               or absolute time specification ``11:20''\n", stderr);
	fputs("       message -> short shutdown message\n", stderr);
	exit(1);
}

static int crashCheck() {
	struct utmp last;
	int fd, crashed;
	struct stat st;

	if (stat(WTMP, &st) < 0 || st.st_size == 0)
	  return 0;
	if ((fd = open(WTMP, O_RDONLY)) < 0)
	  return 0;

	crashed = (lseek(fd, - (off_t) sizeof(last), SEEK_END) == -1 ||
				read(fd, (void *) &last, sizeof(last)) != sizeof(last) ||
				last.ut_line[0] != '~' ||
				(strncmp(last.ut_user, "shutdown", sizeof(last.ut_user)) &&
					strncmp(last.ut_user, "halt", sizeof(last.ut_user))));
	close(fd);
	return crashed;
}

static void parseTime(char *arg) {
	char *p = arg;
	int hours, minutes;
	time_t now;
	struct tm *tm;
	int delta = 0;
	int bad = 0;

	if (p[0] == '+') {
		delta = 1;
		++p;
	}

	hours = strtoul(p, &p, 10);
	if (*p == 0 && delta) {		
		minutes = hours;
		hours = 0;
	} else {
		if (*p != ':' && *p != '.')
		  bad = 1;
		else
		  ++p;
		minutes = strtoul(p, &p, 10);
		if (*p != 0)
		  bad = 1;
	}
	if (bad) {
		fprintf(stderr, "Invalid time specification '%s'\n", arg);
		usageErr();
	}

	time(&now);
	tm = localtime(&now);

	if (! delta) {
		hours -= tm->tm_hour;
		minutes -= tm->tm_min;
	}
	
	if (minutes < 0) {
		minutes += 60;
		--hours;
	}
	if (hours < 0)
	  hours += 24;		/* Time after midnight. */

	tm->tm_hour += hours;
	tm->tm_min += minutes;
	mktime(tm);
	infoHour = tm->tm_hour;
	infoMin = tm->tm_min;

	sprintf(info, 
		"The system will shutdown in %d hour%s and %d minute%s at %02d:%02d\n\n",
		hours, hours == 1 ? "": "s",
		minutes, minutes == 1 ? "" : "s",
		infoHour, infoMin);

	waitTime += hours * 3600 + minutes * 60;
}

static void terminate() {
	FILE *in;
	pid_t pid;
	char c_pid[5];

	in = fopen(SHUT_PID, "r");
	if (in == (FILE *) 0) {
		fputs("Can't get pid of shutdown process, probably not running shutdown\n", stderr);
		exit(1);
	}
	fgets(c_pid, 5, in);
	fclose(in);
	pid = atoi(c_pid);
	if (kill(pid, SIGKILL) == -1)
	  fputs("Can't kill the shutdown process, probably not running anymore\n", stderr);
	else
	  puts("Shutdown process terminated");
	unlink(SHUT_PID);
	unlink(NO_LOGIN);
	exit(0);
}

static void getMessage() {
	char line[80];

	puts("Type your message. End with ^D at an empty line");
	fputs("shutdown> ", stdout);
	fflush(stdout);
	while (fgets(line, 80, stdin) != (char *) 0) {
		strcat(message, line);
		bzero(line, strlen(line));
		fputs("shutdown> ", stdout);
		fflush(stdout);
	}
	putc('\n', stdout);
	fflush(stdout);
}

static int informUserTime() {
	int min;

	min = waitTime / 60;
	if (min == 60 || min == 30 || min == 15 || min == 10 || min <= 5)
	  return 1;
	return 0;
}

static void informUser() {
	int hour, minute;
	char msg[80];

	hour = 0;
	minute = waitTime / 60;
	while (minute >= 60) {
		minute -= 60;
		++hour;
	}

	if (hour)
	  sprintf(msg,
		"\nThe system will shutdown in %d hour%s and %d minute%s at %.02d: %.02d\n\n",
		hour, hour == 1 ? "" : "s", 
		minute, minute == 1 ? "" : "s",
		infoHour, infoMin);
	else if (minute > 1)
	  sprintf(msg,
		"\nThe system will shutdown in %d minutes at %.02d:%.02d\n\n",
		minute, infoHour, infoMin);
	else if (waitTime > 1)
	  sprintf(msg, "\nThe system will shutdown in %d seconds\n\n", waitTime);
	else
	  sprintf(msg, "\nThe system will shutdown NOW\n\n");

	wall(msg, message);
}

static void writePid() {
	char pid[5];
	int fd;

	fd = creat(SHUT_PID, S_IRUSR | S_IWUSR);
	if (! fd)
	  return;
	strncpy(pid, itoa(getpid()), sizeof(pid));
	write(fd, pid, sizeof(pid));
	close(fd);
}

void main(int argc, char **argv) {
	int i, now = 0, noLogin = 0, wantTerminate = 0, wantMessage = 0, check = 0;
	char *opt;
	int tty;
	static char HALT1[] = "-?";
	static char *HALT[] = { "shutdown", HALT1, NULL, NULL };

	/* Parse options. */
	for (i = 1; i < argc && argv[i][0] == '-'; ++i) {
		if (argv[i][1] == '-' && argv[i][2] == 0) {	/* -- */
			++i;
			break;
		}
		for (opt = argv[i] + 1; *opt != 0; ++opt) {
			switch (*opt) {
				case 'k':
					wantTerminate = 1;
					break;
				case 'h':
				case 'r':
				case 'x':
					rebootFlag = *opt;
					if (rebootFlag == 'x') {
						if (*++opt == 0) {
							if (++i == argc) {
								fputs("shutdown: option '-x' requires an argument\n", stderr);
								usageErr();
							}
							opt = argv[i];	/* -x code */
						}	/* else -xcode */
						rebootCode = opt;
						opt = "";
					}
					break;
				case 'R':
					rebootFlag = 'R';
					break;
				case 'm':
					wantMessage = 1;
					break;
				case 'C':
					check = 1;
					break;
				default:
					fprintf(stderr, "shutdown: invalid option '-%c'\n", *opt);
					usageErr();
					break;
			}
		}
	}
	if ((argc - i ) > 2)
		usageErr();

	if (check)
	  exit(crashCheck() ? 0 : 2);

	if (i == argc) {
		/* No timespec, assume "now". */
		now = 1;
	} else {
		if (strcmp(argv[i], "now") == 0) 
		  ++now;
		else
		  parseTime(argv[i]);
	}

	if ((argc - i) == 2) {
		/* One line message */
		strcat(message, argv[i + 1]);
		strcat(message, "\n");
	}

	if (wantTerminate)
	  terminate();
	if (wantMessage)
	  getMessage();

	puts(info);

	prog = getProg(argv);

	if (! now) {
		/* Daemonize. */
		switch (fork()) {
			case 0:		/* Child */
				break;
			case -1:
				fprintf(stderr, "%s: can't fork", prog);
				exit(1);
			default:	/* Parent */
				exit(0);
		}
		/* Detach from the terminal (if any). */
		if ((tty = open("/dev/tty", O_RDONLY)) != -1) {
			close(tty);
			setsid();
		}
		writePid();
	}

	for (;;) {
		if (waitTime <= 5 * 60 && ! noLogin && ! now) {
			close(creat(NO_LOGIN, 00644));	/* create NO_LOGIN file */
			noLogin = 1;
		}
		if (waitTime <= 60)
		  break;
		if (informUserTime())
		  informUser();
		sleep(60);
		waitTime -= 60;
	}

	if (! now) {
		informUser();
		sleep(30);		/* Last minute before shutdown. */
		waitTime -= 30;
		informUser();
		sleep(30);		/* Last 30 seconds before shutdown. */
	}
	waitTime = 0;
	informUser();

	unlink(SHUT_PID);	/* No way of stopping anymore */
	unlink(NO_LOGIN);

	HALT[1][1] = rebootFlag;
	if (rebootFlag == 'x')
	  HALT[2] = rebootCode;
	execv("/usr/bin/halt", HALT);
	if (errno != ENOENT)
	  fprintf(stderr, "Can't execute 'halt': %s\n", strerror(errno));

	sleep(2);
	reboot(RBT_HALT);
	fprintf(stderr, "Reboot call failed: %s\n", strerror(errno));
	exit(1);
}








