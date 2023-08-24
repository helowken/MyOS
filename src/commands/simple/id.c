#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>

void main() {
	struct passwd *pwd;
	struct group *grp;
	uid_t ruid, euid;
	gid_t rgid, egid;

	ruid = getuid();
	euid = geteuid();
	rgid = getgid();
	egid = getegid();

	printf("uid=%d", ruid);
	if ((pwd = getpwuid(ruid)) != NULL)
	  printf("(%s)", pwd->pw_name);

	printf(" gid=%d", rgid);
	if ((grp = getgrgid(rgid)) != NULL)
	  printf("(%s)", grp->gr_name);

	if (euid != ruid) {
		printf(" euid=%d", euid);
		if ((pwd = getpwuid(euid)) != NULL)
		  printf("(%s)", pwd->pw_name);
	}

	if (egid != rgid) {
		printf(" egid=%d", egid);
		if ((grp = getgrgid(egid)) != NULL)
		  printf("(%s)", grp->gr_name);
	}

	printf("\n");
}
