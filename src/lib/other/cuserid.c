#include <lib.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

char *cuserid(char *userName) {
	static char userId[L_cuserid];
	struct passwd *pw;

	if (userName == (char *) NULL)
	  userName = userId;

	pw = getpwuid(geteuid());

	if (pw == (struct passwd *) NULL) {
		userName = '\0';
		return (char *) NULL;
	}
	strcpy(userName, pw->pw_name);

	return userName;
}
