#ifndef _GRP_H
#define _GRP_H

#ifndef _TYPES_H
#include <sys/types.h>
#endif

struct group {
	char *gr_name;		/* The name of the group */
	char *gr_passwd;	/* The group passwd */
	gid_t gr_gid;		/* The numerical group ID */
	char **gr_mem;		/* A vector of pointers to the members */
};

struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *name);

#ifdef _MINIX
void endgrent(void);
struct group *getgrent(void);
int setgrent(void);
void setgrfile(const char *file);
#endif

#endif
