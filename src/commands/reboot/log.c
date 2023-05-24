#define _POSIX_SOURCE	1
#include <sys/types.h>
#include <stdio.h>
#include <utmp.h>
#include <pwd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

static char SHUT_LOG[] = "/usr/adm/log";

static char who[8];
static char *month[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

extern char *prog;

void writeLog() {
	int fd;
	static struct utmp wtmp;
	static struct passwd *pwd;
	char msg[90];
	struct tm *tm;
	time_t now;
	struct utsname utsname;
	char *host = "localhost";

	time(&now);
	tm = localtime(&now);

	if (uname(&utsname) >= 0)
	  host = utsname.nodename;

	pwd = getpwuid(getuid());
	if (pwd == (struct passwd *) 0)
	  strcpy(who, "root");
	else
	  strcpy(who, pwd->pw_name);

	fd = open(WTMP, O_APPEND | O_WRONLY, S_IXOTH);
	if (fd) {
		if (strcmp(prog, "reboot"))
		  strcpy(wtmp.ut_user, prog);
		else
		  strcpy(wtmp.ut_user, "shutdown");	/* last ... */
		strcpy(wtmp.ut_id, "~~");
		strcpy(wtmp.ut_line, "~");
		wtmp.ut_pid = 0;
		wtmp.ut_type = BOOT_TIME;
		wtmp.ut_time = now;
		wtmp.ut_host[0] = '\0';
		write(fd, (char *) &wtmp, sizeof(struct utmp));
		close(fd);
	}
	fd = open(SHUT_LOG, O_APPEND | O_WRONLY, S_IXOTH);
	if (! fd) {
		perror("open");
	} else {
		sprintf(msg, "%s %02d %02d:%02d:%02d %s: system %s by %s@%s\n",
			month[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
			prog, prog, who, host);
		write(fd, msg, strlen(msg));
		close(fd);
	}
}
