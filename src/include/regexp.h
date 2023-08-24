#ifndef _REGEXP_H
#define _REGEXP_H

#define CHARBITS	0377
#define NSUBEXP		10

typedef struct regexp {
	const char *startp[NSUBEXP];
	const char *endp[NSUBEXP];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chuminess with compiler. */
} regexp;

regexp *regcomp(const char *exp);
int regexec(regexp *prog, const char *string, int bolflag);
void regsub(regexp *prog, char *source, char *dest);
void regerror(const char *message);

#endif
