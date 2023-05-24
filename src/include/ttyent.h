#ifndef _TTYENT_H
#define _TTYENT_H

struct ttyent {
	char *ty_name;		/* Name of the terminal device. */
	char *ty_type;		/* Terminal type name (termcap(3)). */
	char **ty_getty;	/* Program to run, normally getty. */
	char **ty_init;		/* Initialization command, normally stty. */
};

struct ttyent *getttyent(void);
struct ttyent *getttynam(const char *name);
int setttyent(void);
void endttyent(void);

#endif

