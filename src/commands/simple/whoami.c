#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void main() {
	struct passwd *pw;

	pw = getpwuid(geteuid());
	if (pw == NULL)
	  exit(EXIT_FAILURE);
	puts(pw->pw_name);
}
