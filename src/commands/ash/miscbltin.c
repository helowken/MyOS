



/* Miscelaneous builtins. */

#include "shell.h"
#include "options.h"
#include "var.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#include <sys/stat.h>
#include <unistd.h>

int readCmd(int argc, char **argv) {
	char **ap;
	int backslash;
	char *prompt;
	char c;
	int eflag;
	char *ifs;
	char *p;
	int startWord;
	int status;
	int i;

	eflag = 0;
	prompt = NULL;
	while ((i = nextOpt("ep:")) != '\0') {
		if (i == 'p')
		  prompt = optArg;
		else
		  eflag = 1;
	}
	if (prompt && isatty(0)) {
		out2Str(prompt);
		flushAll();
	}
	if (*(ap = argPtr) == NULL) 
	  error("arg count");
	if ((ifs = bltinLookup("IFS", 1)) == NULL)
	  ifs = nullStr;
	status = 0;
	startWord = 1;
	backslash = 0;
	START_STACK_STR(p);
	for (;;) {
		if (read(STDIN_FILENO, &c, 1) != 1) {
			status = 1;
			break;
		}
		if (c == '\0')
		  continue;
		if (backslash) {
			backslash = 0;
			if (c != '\n')
			  ST_PUTC(c, p);
			continue;
		}
		if (eflag && c == '\\') {
			++backslash;
			continue;
		}
		if (c == '\n')
		  break;
		if (startWord && *ifs == ' ' && strchr(ifs, c)) 
		  continue;
		startWord = 0;
		if (backslash && c == '\\') {
			if (read(STDIN_FILENO, &c, 1) != 1) {
				status = 1;
				break;
			}
			ST_PUTC(c, p);
		} else if (ap[1] != NULL && strchr(ifs, c) != NULL) {
			STACK_STR_NUL(p);
			setVar(*ap, stackBlock(), 0);
			++ap;
			startWord = 1;
			START_STACK_STR(p);
		} else {
			ST_PUTC(c, p);
		}
	}
	STACK_STR_NUL(p);
	setVar(*ap, stackBlock(), 0);
	while (*++ap != NULL) {
		setVar(*ap, nullStr, 0);
	}
	return status;
}

int umaskCmd(int argc, char **argv) {
	int mask;
	char *p;
	int i;

	if ((p = argv[1]) == NULL) {
		INTOFF;
		mask = umask(0);
		umask(mask);
		INTON;
		out1Format("%.4o\n", mask);	
	} else {
		mask = 0;
		do {
			if ((unsigned) (i = *p - '0') >= 8)
			  error("Illegal number: %s", argv[1]);
			mask = (mask << 3) + i;
		} while (*++p != '\0');
		umask(mask);
	}
	return 0;
}
