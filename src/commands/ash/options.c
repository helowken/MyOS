


#include "shell.h"
#define DEFINE_OPTIONS
#include "options.h"
#undef DEFINE_OPTIONS
#include "input.h"
#include "trap.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"

#include "stdio.h" //TODO

char *arg0;			/* Value of $0 */
ShellParam shellParam;	/* Current positional parameters */
char **argPtr;		/* Argument list for builtin commands */
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
					  xFlag = vFlag = 0;
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
	  sFlag = 1;
	editable = (isatty(0) && isatty(1));
	if (iFlag == 2 && sFlag == 1 && editable)
	  iFlag = 1;
	if (jFlag == 2)
	  jFlag = iFlag;
	for (p = optVal; p < optVal + sizeof(optVal) - 1; ++p) {
		if (*p == 2)
		  *p = 0;
	}
	arg0 = argv[0];
	if (sFlag == 0) {
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
	setInteractive(iFlag);
	//setJobCtl(jFlag);		no job control support
}



