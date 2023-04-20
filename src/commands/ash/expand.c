

#include "unistd.h"
#include "shell.h"
#include "main.h"
#include "nodes.h"
#include "eval.h"
#include "input.h"
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
#include "errno.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "dirent.h"
#include "pwd.h"


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
NodeList *argBackq;			/* List of back quote expressions */
IFSRegion ifsFirst;			/* First struct in list of ifs regions */
IFSRegion *ifsLastPtr;		/* Last struct in list */
ArgList expArg;				/* Holds expanded arg list */

#if UDIR || TILDE
/* Set if the last argument processed had /u/logname or ~logname expanded.
 * This variable is read by the cd command.
 */
int didUserDir;
#endif

static char *expDir;

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
		if (val == NULL || ((flags & VS_NUL) && val[0] == '\0')) {
			val = NULL;
			set = 0;
		} else { 
			set = 1;
		}
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
	if (((flags & VS_QUOTE) == 0 || (*var == '@' && shellParam.numParam != 1)) &&
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
	BackCmd in;
	int i;
	char buf[128];
	char *p;
	char *dest = expDest;
	IFSRegion saveIfs, *saveLastPtr;
	NodeList *saveArgBackq;
	char lastChar;
	int startLoc = dest - stackBlock();
	char const *syntax = quoted ? DQ_SYNTAX : BASE_SYNTAX;
	int saveHereFd;

	INTOFF;
	saveIfs = ifsFirst;
	saveLastPtr = ifsLastPtr;	
	saveArgBackq = argBackq;
	saveHereFd = hereFd;
	hereFd = -1;
	p = grabStackStr(dest);
	evalBackCmd(cmd, &in);
	ungrabStackStr(p, dest);
	ifsFirst = saveIfs;
	ifsLastPtr = saveLastPtr;
	argBackq = saveArgBackq;
	hereFd = saveHereFd;

	p = in.buf;
	lastChar = '\0';
	for (;;) {
		if (--in.numLeft < 0) {
			if (in.fd < 0) 
			  break;
			while ((i = read(in.fd, buf, sizeof(buf))) < 0 && errno == EINTR) {
			}
			if (i <= 0)
			  break;
			p = buf;
			in.numLeft = i - 1;
		}
		lastChar = *p++;
		if (lastChar != '\0') {
			if (full && syntax[(int) lastChar] == CCTL)
			  ST_PUTC(CTL_ESC, dest);
			ST_PUTC(lastChar, dest);
		}
	}
	if (lastChar == '\n')
	  ST_UNPUTC(dest);
	if (in.fd >= 0)
	  close(in.fd);
	if (in.buf)
	  ckFree(in.buf);
	if (in.jp) 
	  exitStatus = waitForJob(in.jp);
	if (quoted == 0)
	  recordRegion(startLoc, dest - stackBlock(), 0);
	expDest = dest;
	INTON;
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

/* Break the argument string into pieces based upon IFS and add the
 * strings to the argument list. The regions of the string to be
 * searched for IFS characters have been stored by recordRegion.
 */
static void ifsBreakup(char *string, ArgList *argList) {
	IFSRegion *irPtr;
	StrList *sp;
	char *start;
	register char *p;
	char *q;
	char *ifs;

	start = string;
	if (ifsLastPtr != NULL) {
		irPtr = &ifsFirst;
		do {
			p = string + irPtr->beginOff;
			ifs = irPtr->nulOnly ? nullStr : ifsVal();
			while (p < string + irPtr->endOff) {
				q = p;
				if (*p == CTL_ESC)
				  ++p;
				if (strchr(ifs, *p++)) {
					if (q > start || *ifs != ' ') {
						*q = '\0';
						sp = (StrList *) stackAlloc(sizeof(*sp));
						sp->text = start;
						*argList->last = sp;
						argList->last = &sp->next;
					}
					if (*ifs == ' ') {	/* Eat the following ifs. */
						for (;;) {
							if (p >= string + irPtr->endOff)
							  break;
							q = p;
							if (*p == CTL_ESC)
							  ++p;
							if (strchr(ifs, *p++) == NULL) {
								p = q;
								break;
							}
						}
					}
					start = p;
				}
			}
		} while ((irPtr = irPtr->next) != NULL);
		if (*start || (*ifs != ' ' && start > string)) {
			sp = (StrList *) stackAlloc(sizeof(*sp));
			sp->text = start;
			*argList->last = sp;
			argList->last = &sp->next;
		}
	} else {
		sp = (StrList *) stackAlloc(sizeof(*sp));
		sp->text = start;
		*argList->last = sp;
		argList->last = &sp->next;
	}
}

/* TILDE: Expand ~username into the home directory for the specified user. 
 * We hope not to use the getpw stuff here, because then we would have to load
 * in stdio and who knows what else. With networked password files there is
 * no choice alas.
 */
#define MAX_LOG_NAME	32
#define MAX_PW_LINE		128

static char *expUserDir(char *path) {
	register char *p, *q, *r;
	char name[MAX_LOG_NAME];
	int i;
	struct passwd *pw;

	r = path;	/* Result on failure */
	p = r + 1;	/* The 1 skips "~" */
	q = name;
	while (*p && *p != '/') {
		if (q >= name + MAX_LOG_NAME - 1)
		  return r;		/* Fail, name too long */
		*q++ = *p++;
	}
	*q = '\0';

	if (*name == 0 && *r == '~') {
		/* null name, use $HOME */
		if ((q = lookupVar("HOME")) == NULL)
		  return r;		/* Fail, home not set */
		i = strlen(q);
		r = stackAlloc(i + strlen(p) + 1);
		scopy(q, r);
		scopy(p, r + i);
		didUserDir = 1;
		return r;
	}
	
	if ((pw = getpwnam(name)) != NULL) {
		/* User exists */
		q = pw->pw_dir;
		i = strlen(q);
		r = stackAlloc(i + strlen(p) + 1);
		scopy(q, r);
		scopy(p, r + i);
		didUserDir = 1;
	}
	endpwent();
	return r;
}

/* Add a file name to the list
 */
static void addFileName(char *name) {
	char *p;
	StrList *sp;

	p = stackAlloc(strlen(name) + 1);
	scopy(name, p);
	sp = (StrList *) stackAlloc(sizeof(*sp));
	sp->text = p;
	*expArg.last = sp;
	expArg.last = &sp->next;
}

/* Do metacharacter (i.e. *, ?, [...]) expansion.
 */
static void expMeta(char *endDir, char *name) {
	register char *p;
	char *q;
	char *start;
	char *endName;
	int metaFlag;
	struct stat sb;
	DIR *dirp;
	struct dirent *dp;
	int atEnd;
	int matchDot;

	metaFlag = 0;
	start = name;
	for (p = name; ; ++p) {
		if (*p == '*' || *p == '?') {
			metaFlag = 1;
		} else if (*p == '[') {
			q = p + 1;
			if (*q == '!')	/* Can't be [!] */
			  ++q;
			for (;;) {
				if (*q == CTL_ESC)
					++q;
				if (*q == '/' || *q == '\0')
				  break;
				if (*++q == ']') {
					metaFlag = 1;
					break;
				}
			}
		} else if (*p == '!' && 
					p[1] == '!' &&
					(p == name || p[-1] == '/')) {
			metaFlag = 1;
		} else if (*p == '\0') {
			break;
		} else if (*p == CTL_ESC) {
			++p;
		}
		if (*p == '/') {
			if (metaFlag)	/* [start, p) has meta: "aa?/bb" */
			  break;
			/* "aa/b?", [start, p) has no meta, start points to "b?" */
			start = p + 1;	
		}
	}
	if (metaFlag == 0) {	/* We've reached the end of the file name */
		if (endDir != expDir)
		  ++metaFlag;
		for (p = name; ; ++p) {
			if (*p == CTL_ESC)
			  ++p;
			*endDir++ = *p;
			if (*p == '\0')
			  break;
		}
		if (metaFlag == 0 || stat(expDir, &sb) >= 0)
		  addFileName(expDir);
		return;
	}
	endName = p;	/* points to the remaining part */ 

	if (start != name) {	
		/* The part of [name, start) has no meta. */
		p = name;
		while (p < start) {
			if (*p == CTL_ESC)
			  ++p;
			*endDir++ = *p++;
		}
	}

	if (endDir == expDir) {	/* ls ab* */
		p = ".";
	} else if (endDir == expDir + 1 && *expDir == '/') {	/* ls /[ab]c */
		p = "/";
	} else {	/* ls aa/bb? */
		p = expDir;
		endDir[-1] = '\0';	/* make "aa/" to "aa" */
	}
	if ((dirp = opendir(p)) == NULL)
	  return;
	if (endDir != expDir)
	  endDir[-1] = '/';		/* make "aa" back to "aa/" */

	/* If *endName is not null, it points to '/' following the meta part. */
	if (*endName == 0) {
		atEnd = 1;
	} else {	
		atEnd = 0;
		*endName++ = '\0';
	}

	matchDot = 0;
	if (start[0] == '.' || 
		(start[0] == CTL_ESC && start[1] == '.'))
	  ++matchDot;
	while (! int_pending() && (dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.' && ! matchDot)
		  continue;
		if (patternMatch(start, dp->d_name)) {
			if (atEnd) {
				scopy(dp->d_name, endDir);
				addFileName(expDir);
			} else {
				for (p = endDir, q = dp->d_name; (*p++ = *q++); ) {
				}
				p[-1] = '/';
				expMeta(p, endName);
			}
		}
	}
	closedir(dirp);
	if (! atEnd)
	  endName[-1] = '/';	/* back to the previous level */
}

static StrList *msort(StrList *list, int len) {
	StrList *p, *q;
	StrList **lpp;
	int half;
	int n;

	if (len <= 1)
	  return list;
	half = len >> 1;
	p = list;
	for (n = half; --n >= 0; ) {
		q = p;
		p = p->next;
	}
	q->next = NULL;		/* Terminate first half of list */
	q = msort(list, half);	/* Sort first half of list */
	p = msort(p, len - half);	/* Sort second half */
	lpp = &list;
	for (;;) {
		if (strcmp(p->text, q->text) < 0) {
			*lpp = p;
			lpp = &p->next;
			if ((p = *lpp) == NULL) {
				*lpp = q;
				break;
			}
		} else {
			*lpp = q;
			lpp = &q->next;
			if ((q = *lpp) == NULL) {
				*lpp = p;
				break;
			}
		}
	}
	return list;
}

/* Sort the results of file name expansion. It calculates the number of
 * strings to sort and then calls msort (Short for merge sort) to do the
 * work.
 */
static StrList *expSort(StrList *str) {
	int len;
	StrList *sp;

	len = 0;
	for (sp = str; sp; sp = sp->next) {
		++len;
	}
	return msort(str, len);
}

/* Expand shell metacharacters. At this point, the only control characters
 * should be escapes. The results are stored in the list expArg.
 */
static void expandMeta(StrList *str) {
	char *p;
	StrList **saveLast;
	StrList *sp;
	char c;

	while (str) {
		if (fflag)
		  goto noMeta;
		p = str->text;
#if TILDE
		if (p[0] == '~')
		  str->text = p = expUserDir(p);
#endif
		for (;;) {		/* Fast check for meta chars */
			if ((c = *p++) == '\0') 
			  goto noMeta;
			if (c == '*' || c == '?' || c == '[' || c == '!')
			  break;
		}
		saveLast = expArg.last;
		INTOFF;
		if (expDir == NULL) {
			int i = strlen(str->text);
			expDir = ckMalloc(i < 2048 ? 2048 : i);
		}
		expMeta(expDir, str->text);
		ckFree(expDir);
		expDir = NULL;
		INTON;
		if (expArg.last == saveLast) {
			/* sh -cz "ls e?", if no item matches "e?", 
			 * it is equal to: sh -cz "ls"
			 */
			if (! zflag) {	
noMeta:
				*expArg.last = str;
				rmEscapes(str->text);
				expArg.last = &str->next;
			}
		} else {
			*expArg.last = NULL;
			*saveLast = sp = expSort(*saveLast);
			while (sp->next != NULL) {
				sp = sp->next;
			}
			expArg.last = &sp->next;
		}
		str = str->next;
	}
}

/* Perform variable substitution and command substitution on an argument,
 * placing the resulting list of arguments in argList. If full is true, 
 * perform splitting and file name expansion. When argList is NULL, perform
 * here document expansion.
 */
void expandArg(Node *arg, ArgList *argList, int full) {
	StrList *sp;
	char *p;

#if UDIR || TILDE
	didUserDir = 0;
#endif
	argBackq = arg->nArg.backquote;
	START_STACK_STR(expDest);
	ifsFirst.next = NULL;
	ifsLastPtr = NULL;
	argStr(arg->nArg.text, full);
	if (argList == NULL)
	  return;		/* Here document expanded */
	ST_PUTC('\0', expDest);
	p = grabStackStr(expDest);
	expArg.last = &expArg.list;
	if (full) {
		ifsBreakup(p, &expArg);
		*expArg.last = NULL;
		expArg.last = &expArg.list;
		expandMeta(expArg.list);
	} else {
		sp = (StrList *) stackAlloc(sizeof(StrList));
		sp->text = p;
		*expArg.last = sp;
		expArg.last = &sp->next;
	}
	while (ifsFirst.next != NULL) {
		IFSRegion *ifsp;
		INTOFF;
		ifsp = ifsFirst.next->next;
		ckFree(ifsFirst.next);
		ifsFirst.next = ifsp;
		INTON;
	}
	*expArg.last = NULL;
	if (expArg.list) {
		*argList->last = expArg.list;
		argList->last = expArg.last;
	}
}

/* See if a pattern matches in a case statement.
 */
int caseMatch(Node *pattern, char *val) {
	StackMark stackMark;
	int result;
	char *p;

	setStackMark(&stackMark);
	argBackq = pattern->nArg.backquote;
	START_STACK_STR(expDest);
	ifsLastPtr = NULL;
	/* Preserve any CTL_ESC characters inserted previously, so that
	 * we won't expand reg exps which are inside strings. */
	argStr(pattern->nArg.text, 1);
	ST_PUTC('\0', expDest);
	p = grabStackStr(expDest);
	result = patternMatch(p, val);
	popStackMark(&stackMark);
	return result;
}

static int pMatch(char *pattern, char *string) {
	register char *p, *q;
	register char c;

	p = pattern;
	q = string;
	for (;;) {
		switch (c = *p++) {
			case '\0':
				goto breakLoop;
			case CTL_ESC:
				if (*q++ != *p++)
				  return 0;
				break;
			case '?':
				if (*q++ == '\0')
				  return 0;
				break;
			case '*':
				c = *p;
				if (c != CTL_ESC && c != '?' && c != '*' && c != '[') {
					while (*q != c) {
						if (*q == '\0')
						  return 0;
						++q;
					}
				}
				/* Check if the last part can be matched. 
				 * For example:
				 *  if pattern == *?abc and string == 123abc:
				 *	  p = ?abc
				 *	  q = 123abc
				 *
				 *  T1 check 123abc (not match)
				 *  T2 check 23abc (not match)
				 *  T3 check 3abc (matches)
				 */
				do {
					if (pMatch(p, q))
					  return 1;
				} while (*q++ != '\0');
				return 0;
			case '[': {
				char *endp;
				int invert, found;
				char chr;

				endp = p;
				if (*endp == '!')
				  ++endp;
				for (;;) {
					if (*endp == '\0')
					  goto dft;		/* No matching ] */
					if (*endp == CTL_ESC)
					  ++endp;
					if (*++endp == ']')
					  break;
				}
				invert = 0;
				if (*p == '!') {
					++invert;
					++p;
				}
				found = 0;
				chr = *q++;
				c = *p++;
				/* Find if chr is in [c] or within [c-p]. 
				 * If found, eat the rest characters before ']'.
				 */
				do {
					if (c == CTL_ESC)
					  c = *p++;
					if (*p == '-' && p[1] != ']') {
						++p;
						if (*p == CTL_ESC)
						  ++p;
						if (chr >= c && chr <= *p)
						  found = 1;
						++p;
					} else {
						if (chr == c)
						  found = 1;
					}
				} while ((c = *p++) != ']');
				if (found == invert)
				  return 0;
				break;
			}
dft:	
			default: 
				if (*q++ != c)
				  return 0;
				break;
		}
	}
breakLoop:
	if (*q != '\0')
	  return 0;
	return 1;
}

/* Returns true if the pattern matches the string.
 */
int patternMatch(char *pattern, char *string) {
	if (pattern[0] == '!' && pattern[1] == '!')
	  return 1 - pMatch(pattern + 2, string);
	return pMatch(pattern, string);
}





