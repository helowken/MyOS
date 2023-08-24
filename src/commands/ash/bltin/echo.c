

#define main	echoCmd

#include "bltin.h"

#undef eflag


int main(int argc, char **argv) {
	register char **ap;
	register char *p;
	register char c;
	int count;
	int nflag = 0;
	int eflag = 0;		/* Enable interpretation of backslash escapes. */

	ap = argv;
	if (argc)
	  ++ap;
	if ((p = *ap) != NULL) {
		if (equal(p, "--")) {
			++ap;
		} else if (*p == '-') {
			while (*++p) {
				if (*p == 'n') 
				  ++nflag;
				else if (*p == 'e') 
				  ++eflag;
				else {
					nflag = 0;
					eflag = 0;
					goto noOpt;
				}
			}
			++ap;
		}
	}
noOpt:
	while ((p = *ap++) != NULL) {
		while ((c = *p++) != '\0') {
			if (c == '\\' && eflag) {
				switch (*p++) {
					case 'b':		/* Backspace*/
						c = '\b';
						break;
					case 'c':		/* Produce no further output */
						return 0;	
					case 'f':		/* Form seed */
						c = '\f';
						break;
					case 'n':		/* New line */
						c = '\n';
						break;
					case 'r':		/* Carriage return */
						c = '\r';
						break;
					case 't':		/* Horizontal tab */
						c = '\t';
						break;
					case 'v':		/* Vertical tab */
						c = '\v';
						break;
					case '\\':		/* Backslash */
						break;		
					case '0':		/* 0NNN: byte with octal value NNN (1 to 3 digits) */
						c = 0;
						count = 3;

						while (--count >= 0 && (unsigned)(*p - '0') < 8) {
							c = (c << 3) + (*p++ - '0');	
						}
						break;
					default:
						--p;
						break;
				}
			}
			putchar(c);
		}
		if (*ap) 
		  putchar(' ');
	}
	if (! nflag)
	  putchar('\n');
	return 0;
}
