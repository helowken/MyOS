#ifndef _PWD_H
#define _PWD_H

#ifndef _TYPES_H
#include <sys/types.h>
#endif

struct passwd {
	char *pw_name;		/* Login name */
	uid_t pw_uid;		/* uid corresponding to the name */
	gid_t pw_gid;		/* gid corresponding to the name */
	char *pw_dir;		/* User's home directory */
	char *pw_shell;		/* Name of the user's shell */

	/* The following members are not defined by POSIX. */
	char *pw_passwd;	/* Password information */
	char *pw_gecos;		/* Just in case you have a GE 645 around */
};

struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);

#ifdef _MINIX
void endpwent(void);
struct passwd *getpwent(void);
int setpwent(void);
void setpwfile(const char *file);
#endif

#endif
