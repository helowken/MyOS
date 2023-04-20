


#include "shell.h"
#define DEFINE_OPTIONS
#include "options.h"
#undef DEFINE_OPTIONS
#include "input.h"
#include "output.h"
#include "trap.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"

char *arg0;			/* Value of $0 */
ShellParam shellParam;	/* Current positional parameters */
char **argPtr;		/* Argument list for builtin commands */
char *optArg;		/* Set by nextOpt (like getopt) */
char *optPtr;		/* Used by nextOpt */
int editable;		/* isatty(0) && isatty(1) */
char *minusC;		/* Argument to -c option */


static void setOption(char flag, int val) {
	register char *p;

	if ((p = strchr(optChar, flag)) == NULL)
	  error("Illegal option -%c", flag);
	optVal[p - optChar] = val;
}

/* Process shell options. The global variable 'argPtr' contains a pointer
 * to the argument list; we advance it past the options.
 */
static void options(int cmdLine) {
	register char *p;
	char *q;
	int val, c;

	if (cmdLine)
	  minusC = NULL;
	while ((p = *argPtr) != NULL) {
		++argPtr;
		if ((c = *p++) == '-') {
			val = 1;
			if (p[0] == '\0' ||		/* For "-" and "--" */
				(p[0] == '-' && p[1] == '\0')) {
				if (! cmdLine) {
					/* "-" means turn off -x and -v */
					if (p[0] == '\0')
					  xflag = vflag = 0;
					/* "--" means reset params */
					else if (*argPtr == NULL)
					  setParam(argPtr);
				}
				break;
			}
		} else if (c == '+') {
			val = 0;
		} else {
			--argPtr;
			break;
		}
		while ((c = *p++) != '\0') {
			if (c == 'c' && cmdLine) { 
				q = *argPtr++;
				if (q == NULL || minusC != NULL)
				  error("Bad -c option");
				minusC = q;
			} else 
			  setOption(c, val);
		}
		if (! cmdLine)
		  break;
	}
}

/* Set the shell parameters.
 */
void setParam(char **argv) {
	char **newParam, **ap;
	int numParam;

	for (numParam = 0; argv[numParam]; ++numParam) {
	}
	ap = newParam = ckMalloc((numParam + 1) * sizeof(*ap));
	while (*argv) {
		*ap++ = saveStr(*argv++);
	}
	*ap = NULL;
	freeParam(&shellParam);
	shellParam.malloc = 1;
	shellParam.numParam = numParam;
	shellParam.params = newParam;
	shellParam.optNext = NULL;
}

/* Free the list of positional parameters.
 */
void freeParam(ShellParam *param) {
	char **ap;

	if (param->malloc) {
		for (ap = param->params; *ap; ++ap) {
			ckFree(*ap);
		}
		ckFree(param->params);
	}
}

/* Standard option processing (a la getopt) for builtin routines. The
 * only argument that is passed to nextOpt is the option string; the
 * other arguments are unnecessary. It return the character, or '\0' on
 * end of input.
 */
int nextOpt(char *optString) {
	register char *p, *q;
	char c;

	if ((p = optPtr) == NULL || *p == '\0') {
		p = *argPtr;
		if (p == NULL || *p != '-' || *++p == '\0') 
		  return '\0';
		++argPtr;
		if (p[0] == '-' && p[1] == '\0')	/* Check for "--" */
		  return '\0';
	}
	c = *p++;
	for (q = optString; *q != c; ) {
		if (*q == '\0')
		  error("Illegal option -%c", c);
		if (*++q == ':') 
		  ++q;
	}
	if (*++q == ':') {	/* For -c100 or -c 100, e.g. */
		if (*p == '\0' && (p = *argPtr) == NULL)
		  error("No arg for -%c option", c);
		optArg = p;
		p = NULL;
	}
	optPtr = p;
	return c;
}

/* Process the shell command line arguments.
 */
void procArgs(int argc, char **argv) {
	char *p;

	argPtr = argv;
	if (argc > 0)
	  ++argPtr;
	/* "optVal + sizeof(optVal) - 1" excludes '\0' */
	for (p = optVal; p < optVal + sizeof(optVal) - 1; ++p) {	
		*p = 2;
	}
	options(1);
	if (*argPtr == NULL && minusC == NULL)
	  sflag = 1;
	editable = (isatty(0) && isatty(1));
	if (iflag == 2 && sflag == 1 && editable)
	  iflag = 1;
	if (jflag == 2)
	  jflag = iflag;
	for (p = optVal; p < optVal + sizeof(optVal) - 1; ++p) {
		if (*p == 2)
		  *p = 0;
	}
	arg0 = argv[0];
	if (sflag == 0) {
		arg0 = *argPtr++;
		if (minusC == NULL) {
			commandName = arg0;
			setInputFile(commandName, 0);
		}
	}
	shellParam.params = argPtr;
	while (*argPtr) {
		++shellParam.numParam;
		++argPtr;
	}
	setInteractive(iflag);
	//setJobCtl(jflag);		no job control support
}

/* The set command builtin. */
int setCmd(int argc, char **argv) {
	if (argc == 1)
	  return showVarsCmd(argc, argv);
	INTOFF;
	options(0);
	setInteractive(iflag);
	if (*argPtr != NULL)
	  setParam(argPtr);
	INTON;
	return 0;
}

/* The getOpts builtin. ShellParam.optNext points to the next argument
 * to be processed. ShellParam.optPtr points to the next character to
 * be processed in the current argument. If ShellParam.optNext is NULL,
 * then it's the first time getOpts has been called.
 */
int getOptsCmd(int argc, char **argv) {
	register char *p, *q;
	char c;
	char s[10];

	if (argc != 3)
	  error("Usage: getopts optString var");
	if (shellParam.optNext == NULL) {
		shellParam.optNext = shellParam.params;
		shellParam.optPtr = NULL;
	}
	if ((p = shellParam.optPtr) == NULL || *p == '\0') {
		p = *shellParam.optNext;
		if (p == NULL || *p != '-' || *++p == '\0') {	/* No options */
atEnd:
			formatStr(s, 10, "%d", shellParam.optNext - shellParam.params + 1);
			setVar("OPTIND", s, 0);
			shellParam.optNext = NULL;
			return 1;
		}
		++shellParam.optNext;
		if (p[0] == '-' && p[1] == '\0')	/* Check for "--" */
		  goto atEnd;
	}
	/* Check if option is included in optString. */
	c = *p++;
	for (q = argv[1]; *q != c; ) {
		if (*q == '\0') {
			out1Format("Illegal option -%c\n", c);
			c = '?';
			goto out;
		}
		if (*++q == ':')
		  ++q;
	}
	if (*++q == ':') {	/* Option value is needed. */
		if (*p == '\0') {
			if ((p = *shellParam.optNext) == NULL) {
				out1Format("No arg for -%c option\n", c);
				c = '?';
				goto out;
			}
			++shellParam.optNext;
		}
		setVar("OPTARG", p, 0);
		p = "";
	}
out:
	shellParam.optPtr = p;
	s[0] = c;
	s[1] = '\0';
	setVar(argv[2], s, 0);
	formatStr(s, 10, "%d", shellParam.optNext - shellParam.params + 1);
	setVar("OPTIND", s, 0);
	return 0;
}

/* The shift builtin command. */
int shiftCmd(int argc, char **argv) {
	int n;
	char **ap1, **ap2;

	n = 1;
	if (argc > 1)
	  n = number(argv[1]);
	if (n > shellParam.numParam)
	  n = shellParam.numParam;

	INTOFF;
	shellParam.numParam -= n;
	for (ap1 = shellParam.params; --n >= 0; ++ap1) {
		if (shellParam.malloc)
		  ckFree(*ap1);
	}
	ap2 = shellParam.params;
	while ((*ap2++ = *ap1++) != NULL) {
	}
	shellParam.optNext = NULL;
	INTON;
	return 0;
}


#ifdef mkinit
INCLUDE "options.h"

SHELLPROC {
	char *p;
	for (p = optVal; p < optVal + sizeof(optVal); ++p) {
		*p = 0;
	}
}
#endif

