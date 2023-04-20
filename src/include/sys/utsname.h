#ifndef _UTSNAME_H
#define _UTSNAME_H

struct utsname {
	char sysname[15 + 1];
	char nodename[255 + 1];
	char release[11 + 1];
	char version[7 + 1];
	char machine[11 + 1];
	char arch[11 + 1];
};

int uname(struct utsname *name);

#endif
