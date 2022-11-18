

#include "shell.h"
#include "main.h"
#include "nodes.h"
#include "eval.h"
#include "expand.h"
#include "syntax.h"
#include "parser.h"
#include "jobs.h"
#include "options.h"
#include "memalloc.h"
#include "mystring.h"
#include "var.h"
#include "output.h"
#include "error.h"


/* Structure specifying which parts of the string should be searched
 * for IFS characters.
 */
typedef struct IFSRegion {
	struct IFSRegion *next;	/* Next region in list */
	int beginOff;			/* Offset of start of region */
	int endOff;				/* Offset of end of region */
	int nulOnly;			/* Search for nul bytes only */
} IFSRegion;


char *expDest;				/* Output of current string */
NodeList *argBackq;		/* List of back quote expressions */
IFSRegion ifsFirst;			/* First struct in list of ifs regions */
IFSRegion *ifsLastPtr;			/* Last struct in list */
ArgList expArg;				/* Holds expanded arg list */

#if UDIR || TILDE
/* Set if the last argument processed had /u/logname or ~logname expanded.
 * This variable is read by the cd command.
 */
int didUDir;
#endif

static void argStr(char *p, int full);


/* Remove any CTL_ESC characters from a string. */
void rmEscapes(char *str) {
	register char *p, *q;

	p = str;
	while (*p != CTL_ESC) {
		if (*p++ == '\0')
		  return;
	}
	q = p;
	while (*p) {
		if (*p == CTL_ESC)
		  ++p;
		*q++ = *p++;
	}
	*q = '\0';
}

/* Test whether a specialized variable is set. 
 */
static int varIsSet(char name) {
	char **ap;

	if (name == '!') {
		if (backgndPid == -1)
		  return 0;
	} else if (name == '@' || name == '*') {
		if (*shellParam.params == NULL)
		  return 0;
	} else if ((unsigned) (name -= '1') <= '9' - '1') {
		ap = shellParam.params;
		do {
			if (*ap++ == NULL)
			  return 0;
		} while (--name >= 0);	
	}
	return 1;
}

/* Add the value of a specialized variable to the stack string.
 */
static void varValue(char name, int quoted, int allowSplit) {
	int num;
	char temp[32];
	char *p;
	int i;
	extern int exitStatus;
	char sep;
	char **ap;
	char const *syntax;

	switch (name) {
		case '$':
			num = rootPid;
			goto numVar;
		case '?':
			num = exitStatus;
			goto numVar;
		case '#':
			num = shellParam.numParam;
			goto numVar;
		case '!':
			num = backgndPid;
numVar:
			p = temp + 31;
			temp[31] = '\0';
			do {
				*--p = num % 10 + '0';
			} while ((num /= 10) != 0);
			while (*p) {
				ST_PUTC(*p++, expDest);
			}
			break;
		case '-':
			for (i = 0; optChar[i]; ++i) {
				if (optVal[i])
				  ST_PUTC(optChar[i], expDest);
			}
			break;
		case '@':
			if (allowSplit) {
				sep = '\0';
				goto allArgs;
			}
			/* Fall through */
		case '*':
			sep = ' ';
allArgs:
			/* Only emit CTL_ESC if we will do further processing,
			 * i.e. if allowSplit is set. */
			syntax = quoted && allowSplit ? DQ_SYNTAX : BASE_SYNTAX;
			for (ap = shellParam.params; (p = *ap++) != NULL; ) {
				/* Should insert CTL_ESC characters */
				while (*p) {
					if (syntax[(int) *p] == CCTL)
					  ST_PUTC(CTL_ESC, expDest);
					ST_PUTC(*p++, expDest);
				}
				if (*ap)
				  ST_PUTC(sep, expDest);
			}
			break;
		case '0':
			p = arg0;
string:
			/* Only emit CTL_ESC if we will do further processing,
			 * i.e. if allowSplit is set. */
			syntax = quoted && allowSplit ? DQ_SYNTAX : BASE_SYNTAX;
			while (*p) {
				if (syntax[(int) *p] == CCTL)
				  ST_PUTC(CTL_ESC, expDest);
				ST_PUTC(*p++, expDest);
			}
			break;
		default:
			if ((unsigned) (name -= '1') <= '9' - '1') {
				p = shellParam.params[(int) name];
				goto string;
			}
	}
}

static void recordRegion(int start, int end, int nulOnly) {
	register IFSRegion *ifsp;

	if (ifsLastPtr == NULL) {
		ifsp = &ifsFirst;
	} else {
		ifsp = (IFSRegion *) ckMalloc(sizeof(IFSRegion));
		ifsLastPtr->next = ifsp;
	}
	ifsLastPtr = ifsp;
	ifsLastPtr->next = NULL;
	ifsLastPtr->beginOff = start;
	ifsLastPtr->endOff = end;
	ifsLastPtr->nulOnly = nulOnly;
}

/* Expand a variable, and return a pointer to the next character in the
 * input string.
 */
static char *evalVar(char *p, int full) {
	int subType;
	int flags;
	char *var, *val;
	int c;
	int set;
	int special;
	int startLoc;

	flags = *p++;
	subType = flags & VS_TYPE;
	var = p;
	special = 0;
	if (! isName(*p))
	  special = 1;
	p = strchr(p, '=') + 1;
again:	/* Jump here after setting a variable with ${var=text} */
	if (special) {
		set = varIsSet(*var);
		val = NULL;
	} else {
		val = lookupVar(var);
		if (val == NULL || ((flags & VS_NUL) && val[0] == '\0')) {	//TODO
			val = NULL;
			set = 0;
		} else 
		  set = 1;
	}
	startLoc = expDest - stackBlock();
	if (set && subType != VS_PLUS) {
		/* Insert the value of the variable. */
		if (special) {
			varValue(*var, flags & VS_QUOTE, full);
		} else {
			char const *syntax = (flags & VS_QUOTE) ? DQ_SYNTAX : BASE_SYNTAX;
			while (*val) {
				if (full && syntax[(int) *val] == CCTL)
				  ST_PUTC(CTL_ESC, expDest);
				ST_PUTC(*val++, expDest);
			}
		}
	}
	if (subType == VS_PLUS)
	  set = ! set;
	if (((flags & VS_QUOTE) == 0 || 
			(*var == '@' && shellParam.numParam != 1)) &&
			(set || subType == VS_NORMAL))
	  recordRegion(startLoc, expDest - stackBlock(), flags & VS_QUOTE);
	if (! set && subType != VS_NORMAL) {
		if (subType == VS_PLUS || subType == VS_MINUS) {
			argStr(p, full);
		} else {
			char *startp;
			int saveHereFd = hereFd;
			hereFd = -1;
			argStr(p, 0);
			STACK_STR_NUL(expDest);
			hereFd = saveHereFd;
			startp = stackBlock() + startLoc;
			if (subType == VS_ASSIGN) {
				setVar(var, startp, 0);
				ST_ADJUST(startp - expDest, expDest);
				flags &= ~VS_NUL;
				goto again;
			}
			/* subType == VS_QUESTION */
			if (*p != CTL_END_VAR) {
				outFormat(&errOut, "%s\n", startp);
				error((char *) NULL);
			}
			error("%.*s: parameter %snot set", p - var - 1,
				var, (flags & VS_NUL) ? "null or " : nullStr);
		}
	}
	if (subType != VS_NORMAL) {		/* Skip to end of alternative */
		int nesting = 1;
		for (;;) {
			if ((c = *p++) == CTL_ESC)
			  ++p;
			else if (c == CTL_BACK_Q || c == (CTL_BACK_Q | CTL_QUOTE)) {
				if (set) {
					if (argBackq != NULL)
					  argBackq = argBackq->next;
				}
			} else if (c == CTL_VAR) {
				if ((*p++ & VS_TYPE) != VS_NORMAL)
				  ++nesting;
			} else if (c == CTL_END_VAR) {
				if (--nesting == 0)
				  break;
			}
		}
	}
	return p;
}

/* Expand stuff in backwards quotes.
 */
static void expBackquote(Node *cmd, int quoted, int full) {
	//TODO
}

/* Perform variable and command substitution. If full is set, output CTL_ESC
 * characters to allow for further processing. If full if not set, treat
 * $@ like $* since no splitting will be performed.
 */
static void argStr(register char *p, int full) {
	char c;

	for (;;) {
		switch (c = *p++) {
			case '\0':
			case CTL_END_VAR:
				goto breakLoop;
			case CTL_ESC:
				if (full)
				  ST_PUTC(c, expDest);
				c = *p++;
				ST_PUTC(c, expDest);
				break;
			case CTL_VAR:
				p = evalVar(p, full);
				break;
			case CTL_BACK_Q:
			case CTL_BACK_Q | CTL_QUOTE:
				expBackquote(argBackq->node, c & CTL_QUOTE, full);
				argBackq = argBackq->next;
				break;
			default:
				ST_PUTC(c, expDest);
		}
	}
breakLoop:;
}

/* Perform variable substitution and command substitution on an argument,
 * placing the resulting list of arguments in argList. If full is true, 
 * perform splitting and file name expansion. When argList is NULL, perform
 * here document expansion.
 */
void expandArg(Node *arg, ArgList *argList, int full) {
	//StrList *sp;
	//char *p;

#if UDIR || TILDE
	didUDir = 0;
#endif
	argBackq = arg->nArg.backquote;
	START_STACK_STR(expDest);
	ifsFirst.next = NULL;
	ifsLastPtr = NULL;
	argStr(arg->nArg.text, full);
	//TODO
}









