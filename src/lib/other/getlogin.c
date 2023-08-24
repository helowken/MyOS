#include <lib.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#ifndef	L_cuserid
#define L_cuserid	9
#endif

char *getlogin() {
	static char userId[L_cuserid];
	struct passwd *pwd;

	pwd = getpwuid(getuid());
	if (pwd == (struct passwd *) NULL)
	  return (char *) NULL;

	strcpy(userId, pwd->pw_name);
	return userId;
}
