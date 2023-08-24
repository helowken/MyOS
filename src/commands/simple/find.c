#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>

/* Definitions */
#define SHELL			"/bin/sh"
#define MAX_ARG			256	/* Maximum length for an argv for -exec */
#define BSIZE			512	/* POSIX wants 512 byte blocks */
#define SECS_PER_DAY	(24L * 60L * 60L)	

#define OP_NAME			1	/* Match name */
#define OP_PERM			2	/* Check file permission bits */
#define OP_TYPE			3	/* Check file type bits */
#define OP_LINKS		4	/* Check link count */
#define OP_USER			5	/* Check owner */
#define OP_GROUP		6	/* Check group ownership */
#define OP_SIZE			7	/* Check size, blocks or bytes */
#define OP_SIZEC		8	/* This is a fake for -size with 'c' */
#define OP_INUM			9	/* Compare inode number */
#define OP_ATIME		10	/* Check last access time */
#define OP_CTIME		11	/* Check creation time */
#define OP_MTIME		12	/* Check last modification time */
#define OP_EXEC			13	/* Execute command */
#define OP_OK			14	/* Execute with confirmation */
#define OP_PRINT		15	/* Print name */
#define OP_PRINT0		16	/* Print name null terminated */
#define OP_NEWER		17	/* Compare modification times */
#define OP_AND			18	/* Logical and (short circuit) */
#define OP_OR			19	/* Logical or (short circuit) */
#define OP_XDEV			20	/* Do not cross file-system boundaries */
#define OP_DEPTH		21	/* Descend directory before testing */
#define OP_PRUNE		22	/* Don't descend into current directory */
#define OP_NOUSER		23	/* Check validity of user id */
#define OP_NOGROUP		24	/* Check validity of group id */
#define OP_MAXDEPTH		25	/* Check max depths */
#define LPAR			26	/* Left parenthesis */
#define RPAR			27	/* Right parenthesis */
#define NOT				28	/* Logical not */

/* Some return values: */
#define EOI				-1	/* End of expression */
#define NONE			0	/* Not a valid predicate */

/* For -perm with symbolic modes: */
#define ISWHO(c)		((c == 'u') || (c == 'g') || (c == 'o') || (c == 'a'))
#define ISOPER(c)		((c == '-') || (c == '=') || (c == '+'))
#define ISMODE(c)		((c == 'r') || (c == 'w') || (c == 'x') || (c == 's') || (c == 't'))
#define MUSER			1
#define MGROUP			2
#define MOTHERS			4


typedef struct {
	int e_cnt;
	char *e_vec[MAX_ARG];
} Exec;

typedef struct Node {
	int n_type;		/* Any OP_ or NOT */
	union {
		char *n_str;
		struct {
			long n_val;
			int n_sign;
		} n_int;
		Exec *n_exec;
		struct {
			struct Node *n_left;
			struct Node *n_right;
		} n_opnd;
	} n_info;
} Node;

typedef struct {
	char *op_str;
	int op_val;
} Oper;

static Oper ops[] = {
	{ "name",		OP_NAME },
	{ "perm",		OP_PERM },
	{ "type",		OP_TYPE },
	{ "links",		OP_LINKS },
	{ "user",		OP_USER },
	{ "group",		OP_GROUP },
	{ "size",		OP_SIZE },
	{ "inum",		OP_INUM },
	{ "atime",		OP_ATIME },
	{ "ctime",		OP_CTIME },
	{ "mtime",		OP_MTIME },
	{ "exec",		OP_EXEC },
	{ "ok",			OP_OK },
	{ "print",		OP_PRINT },
	{ "print0",		OP_PRINT0 },
	{ "newer",		OP_NEWER },
	{ "a",			OP_AND },
	{ "o",			OP_OR },
	{ "xdev",		OP_XDEV },
	{ "depth",		OP_DEPTH },
	{ "prune",		OP_PRUNE },
	{ "nouser",		OP_NOUSER },
	{ "nogroup",	OP_NOGROUP },
	{ "maxdepth",	OP_MAXDEPTH },
	{ 0, 0 }
};

static char **ipp;			/* Pointer to next argument during parsing */
static char *prog;
static char *epath;			/* Value of PATH environment string */
static long currentTime;	
static int tty;				/* fd for /dev/tty when using -ok */
static int xdevFlag = 0;	/* Cross device boundaries? */
static int devNum;			/* Device num of first inode */
static int depthFlag = 0;	/* Descend before check? (depth-first order?) */
static int pruneHere;		/* This is Baaaad! Don't ever do this again! */
static int um;				/* Current umask() */
static int needPrint = 1;	/* Implicit -print needed? */


static Node *expr(int t);


static void nonFatal(char *s1, char *s2) {
	fprintf(stderr, "%s: %s%s\n", prog, s1, s2);
}

static void fatal(char *s1, char *s2) {
	nonFatal(s1, s2);
	exit(1);
}

static char *Malloc(int n) {
	char *m;

	if ((m = (char *) malloc(n)) == NULL)
	  fatal("out of memory", "");
	return m;
}

static char *Salloc(char *s) {
	return strcpy(Malloc(strlen(s) + 1), s);
}

static void checkArg(char *arg) {
	if (arg == 0)
	  fatal("syntax error, argument expected", "");
}

static int isNumber(register char *str, int base, int sign) {
	if (sign && (*str == '-' || (base == 8 && *str == '+')))
	  ++str;
	while (*str >= '0' && *str < '0' + base) {
		++str;
	}
	return *str == '\0' ? 1 : 0;
}

static void number(char *str, int base, long *pl, int *ps) {
	int up = '0' + base - 1;
	long val = 0;

	if (*str == '-') {
		*ps = -1;
		++str;
	} else if (*str == '+') {
		*ps = 1;
		++str;
	} else 
	  *ps = 0;

	while (*str >= '0' && *str <= up) {
		val = base * val + (*str++ - '0');
	}
	if (*str)
	  fatal("syntax error: illegal numeric value", "");
	*pl = val;
}

/* Don't try to understand the following one... */
int sMatch(char *s, char *t) {	/* Shell-like matching */
	register int n;

	if (*t == '\0')
	  return *s == '\0';
	if (*t == '*') {	/* "a*b" */
		++t;
		do {
			if (sMatch(s, t))
			  return 1;
		} while (*s++ != '\0');
		return 0;
	}
	if (*s == '\0') return 0;
	if (*t == '\\') return *s == *++t ? sMatch(++s, ++t) : 0;	/* "a\.b" */
	if (*t == '?') return sMatch(++s, ++t);		/* "a?b" */
	if (*t == '[') {
		while (*++t != ']') {
			if (*t == '\\')
			  ++t;
			if (*(t + 1) != '-') {
				if (*t == *s) {		/* [ab] */
					while ((*++t != ']')) {	
						if (*t == '\\')
						  ++t;
					}
					return sMatch(++s, ++t);
				} else {
					continue;
				}
			}
			if (*(t + 2) == ']')	/* "[a-]" */
			  return *s == *t || *s == '-';
			n = (*(t + 2) == '\\') ? 3 : 2;
			if (*s >= *t && *s <= *(t + n)) {	/* [a-z] or [*-\.] */
				while ((*++t != ']')) {
					if (*t == '\\')
					  ++t;
				}
				return sMatch(++s, ++t);
			}
			t += n;
		}
		return 0;
	}
	return *s == *t ? sMatch(++s, ++t) : 0;
}


/* PARSER */
/* Grammer:
 * expr			: primary | primary OR expr;
 * primary		: secondary | secondary AND primary;
 * secondary	: NOT secondary | LPAR expr RPAR | simple;
 * simple		: -OP args...
 */

static int lex(char *str) {
	if (str == NULL)
	  return EOI;
	if (*str == '-') {
		register Oper *op;

		++str;
		for (op = ops; op->op_str; ++op) {
			if (strcmp(str, op->op_str) == 0)
			  break;
		}
		return op->op_val;
	}
	if (str[1] == 0) {
		switch (*str) {
			case '(': return LPAR;
			case ')': return RPAR;
			case '!': return NOT;
		}
	}
	return NONE;
}

static Node *newNode(int t) {
	Node *n = (Node *) Malloc(sizeof(Node));
	n->n_type = t;
	return n;
}

static void doMode(int op, int *mode, int bits) {
	switch (op) {
		case '-':
			*mode &= ~bits;		/* Clear bits */
			break;
		case '=':
			*mode |= bits;		/* Set bits */
			break;
		case '+':
			*mode |= (bits & ~um);	/* Set, but take umask in account */
			break;
	}
}

static void fmode(char *str, long *pl, int *ps) {
	int m = 0, w, op;
	char *p = str;

	if (*p == '-') {
		*ps = -1;
		++p;
	} else {
		*ps = 0;
	}

	while (*p) {
		w = 0;
		if (ISOPER(*p))
		  w = MUSER | MGROUP | MOTHERS;
		else if (! ISWHO(*p))
		  fatal("u, g, o, or a expected: ", p);
		else {
			while (ISWHO(*p)) {
				switch (*p) {
					case 'u':
						w |= MUSER;
						break;
					case 'g':
						w |= MGROUP;
						break;
					case 'o':
						w |= MOTHERS;
						break;
					case 'a': 
						w = MUSER | MGROUP | MOTHERS;
						break;
				}
				++p;
			}
			if (! ISOPER(*p))
			  fatal("-, + or = expected: ", p);
		}
		op = *p++;
		while (ISMODE(*p)) {
			switch (*p) {
				case 'r':
					if (w & MUSER) doMode(op, &m, S_IRUSR);
					if (w & MGROUP) doMode(op, &m, S_IRGRP);
					if (w & MOTHERS) doMode(op, &m, S_IROTH);
					break;
				case 'w':
					if (w & MUSER) doMode(op, &m, S_IWUSR);
					if (w & MGROUP) doMode(op, &m, S_IWGRP);
					if (w & MOTHERS) doMode(op, &m, S_IWOTH);
					break;
				case 'x':
					if (w & MUSER) doMode(op, &m, S_IXUSR);
					if (w & MGROUP) doMode(op, &m, S_IXGRP);
					if (w & MOTHERS) doMode(op, &m, S_IXOTH);
					break;
				case 's':
					if (w & MUSER) doMode(op, &m, S_ISUID);
					if (w & MGROUP) doMode(op, &m, S_ISGID);
					break;
				case 't':
					doMode(op, &m, S_ISVTX);
					break;
			}
			++p;
		}
		if (*p) {
			if (*p == ',')
			  ++p;
			else
			  fatal("garbage at end of mode string: ", p);
		}
	}
	*pl = m;
}

static char *findBin(char *s) {
	char *f, *l, buf[PATH_MAX];

	if (*s == '/')
	  return access(s, X_OK) == 0 ? s : NULL;
	l = f = epath;
	for (;;) {
		if (*l == ':' || *l == 0) {
			if (f == l) {	/* Check in urrent dir */
				if (access(s, X_OK) == 0)
				  return Salloc(s);
				++f;
			} else {	/* Check in epath */
				register char *p = buf, *q = s;
				while (f != l) {
					*p++ = *f++;
				}
				++f;
				*p++ = '/';
				while ((*p++ = *q++)) {
				}
				if (access(buf, X_OK) == 0)
				  return Salloc(buf);
			}
			if (*l == 0)
			  break;
		}
		++l;
	}
	return NULL;
}

static Node *simple(int t) {
	Node *p = newNode(t);
	Exec *e;
	struct stat st;
	struct passwd *pw;
	struct group *gr;
	long l;
	int i;

	switch (t) {
		case OP_TYPE:
			checkArg(*++ipp);
			switch (**ipp) {
				case 'b':
					p->n_info.n_int.n_val = S_IFBLK;
					break;
				case 'c':
					p->n_info.n_int.n_val = S_IFCHR;
					break;
				case 'd':
					p->n_info.n_int.n_val = S_IFDIR;
					break;
				case 'f':
					p->n_info.n_int.n_val = S_IFREG;
					break;
				case 'l':
					p->n_info.n_int.n_val = S_IFLNK;
					break;
				default:
					fatal("-type needs b, c, d, f or l", "");
			}
			break;
		case OP_USER:
			checkArg(*++ipp);
			if (((pw = getpwnam(*ipp)) == NULL) &&
						isNumber(*ipp, 10, 0)) {
				number(*ipp, 10, &(p->n_info.n_int.n_val), 
							&(p->n_info.n_int.n_sign));
			} else {
				if (pw == NULL)
				  fatal("unknown user: ", *ipp);
				p->n_info.n_int.n_val = pw->pw_uid;
				p->n_info.n_int.n_sign = 0;
			}
			break;
		case OP_GROUP:
			checkArg(*++ipp);
			if (((gr = getgrnam(*ipp)) == NULL) &&
						isNumber(*ipp, 10, 0)) {
				number(*ipp, 10, &(p->n_info.n_int.n_val), 
							&(p->n_info.n_int.n_sign));
			} else {
				if (gr == NULL)
				  fatal("unknown group: ", *ipp);
				p->n_info.n_int.n_val = gr->gr_gid;
				p->n_info.n_int.n_sign = 0;
			}
			break;
		case OP_SIZE:
			checkArg(*++ipp);
			i = strlen(*ipp) - 1;
			if ((*ipp)[i] == 'c') {
				p->n_type = OP_SIZEC;	/* Count in bytes i.s.o. blocks */
				(*ipp)[i] = '\0';
			}
			number(*ipp, 10, &(p->n_info.n_int.n_val), 
						&(p->n_info.n_int.n_sign));
			break;
		case OP_LINKS:
		case OP_INUM:
			checkArg(*++ipp);
			number(*ipp, 10, &(p->n_info.n_int.n_val), 
						&(p->n_info.n_int.n_sign));
			break;
		case OP_PERM:
			checkArg(*++ipp);
			if (isNumber(*ipp, 8, 1))
			  number(*ipp, 10, &(p->n_info.n_int.n_val), 
						  &(p->n_info.n_int.n_sign));
			else
			  fmode(*ipp, &(p->n_info.n_int.n_val), 
						  &(p->n_info.n_int.n_sign));
			break;
		case OP_ATIME:
		case OP_CTIME:
		case OP_MTIME:
			checkArg(*++ipp);
			number(*ipp, 10, &l, &(p->n_info.n_int.n_sign));
			p->n_info.n_int.n_val = currentTime - l * SECS_PER_DAY;
			/* More than n days old means less than the absolute time */
			p->n_info.n_int.n_sign *= -1;
			break;
		case OP_EXEC:
		case OP_OK:
			checkArg(*++ipp);
			e = (Exec *) Malloc(sizeof(Exec));
			e->e_cnt = 2;
			e->e_vec[0] = SHELL;
			p->n_info.n_exec = e;
			while (*ipp) {
				if (**ipp == ';' && (*ipp)[1] == '\0') {
					e->e_vec[e->e_cnt] = 0;
					break;
				}
				e->e_vec[(e->e_cnt)++] = (**ipp == '{' && (*ipp)[1] == '}' &&
							(*ipp)[2] == '\0') ? (char *) -1 : *ipp;
				++ipp;
			}
			if (*ipp == 0)
			  fatal("-exec/-ok: ; missing", "");
			if ((e->e_vec[1] = findBin(e->e_vec[2])) == NULL)
			  fatal("can't find program ", e->e_vec[2]);
			if (t == OP_OK) {
				if ((tty = open("/dev/tty", O_RDWR)) < 0)
				  fatal("can't open /dev/tty", "");
			}
			break; 
		case OP_NEWER: 
			checkArg(*++ipp);
			if (lstat(*ipp, &st) == -1)
			  fatal("-newer: can't get status of ", *ipp);
			p->n_info.n_int.n_val = st.st_mtime;
			break;
		case OP_NAME:
			checkArg(*++ipp);
			p->n_info.n_str = *ipp;
			break;
		case OP_XDEV:
			xdevFlag = 1;
			break;
		case OP_DEPTH:
			depthFlag = 1;
			break;
		case OP_MAXDEPTH:
			checkArg(*++ipp);
			number(*ipp, 10, &(p->n_info.n_int.n_val), 
						&(p->n_info.n_int.n_sign));
			break;
		case OP_PRUNE:
		case OP_PRINT:
		case OP_PRINT0:
		case OP_NOUSER:
		case OP_NOGROUP:
			break;
		default:
			fatal("syntax error, operator expected", "");
	}
	if (t == OP_PRINT || t == OP_PRINT0 || t == OP_EXEC || t == OP_OK)
	  needPrint = 0;

	return p;
}

static Node *secondary(int t) {
	Node *n, *p;

	if (t == LPAR) {
		n = expr(lex(*++ipp));
		if (lex(*++ipp) != RPAR)
		  fatal("syntax error, ) expected", "");
		return n;
	}
	if (t == NOT) {
		n = secondary(lex(*++ipp));
		p = newNode(NOT);
		p->n_info.n_opnd.n_left = n;
		return p;
	}
	return simple(t);
}

static Node *primary(int t) {
	Node *n, *p, *n2;

	n = secondary(t);
	if ((t = lex(*++ipp)) != OP_AND) {
		--ipp;
		if (t == EOI || t == RPAR || t == OP_OR)
		  return n;
	}
	n2 = primary(lex(*++ipp));
	p = newNode(OP_AND);
	p->n_info.n_opnd.n_left = n;
	p->n_info.n_opnd.n_right = n2;
	return p;
}

static Node *expr(int t) {
	Node *n, *p, *n2;

	n = primary(t);
	if ((t = lex(*++ipp)) == OP_OR) {
		n2 = expr(lex(*++ipp));
		p = newNode(OP_OR);
		p->n_info.n_opnd.n_left = n;
		p->n_info.n_opnd.n_right = n2;
		return p;
	}
	--ipp;
	return n;
}

static int ichk(long val, Node *n) {
	switch (n->n_info.n_int.n_sign) {
		case 0:
			return val == n->n_info.n_int.n_val;
		case 1:
			return val > n->n_info.n_int.n_val;
		case -1:
			return val < n->n_info.n_int.n_val;
	}
	fatal("internal: bad n_sign", "");
	return 0;
}

static int execute(int op, Exec *e, char *path) {
	int s, pid;
	char *argv[MAX_ARG];
	register char **p, **q;

	for (p = e->e_vec, q = argv; *p; ) {	/* Replace the {}s */
		if ((*q++ = *p++) == (char *) -1)
		  q[-1] = path;
	}
	*q = '\0';

	if (op == OP_OK) {
		char answer[10];
		
		/* 0: /bin/sh
		 * 1: full path of exec
		 */
		for (p = &argv[2]; *p; ++p) {
			write(tty, *p, strlen(*p));
			write(tty, " ", 1);
		}
		write(tty, "? ", 2);
		if (read(tty, answer, 10) < 2 || *answer != 'y')
		  return 0;
	}

	if ((pid = fork()) == -1)
	  fatal("can't fork", "");
	if (pid == 0) {
		register int i = 3;

		while (close(i++) == 0) {
		}
		execv(argv[1], &argv[2]);	/* Binary itself? */
		execv(argv[0], &argv[1]);	/* Shell command? */
		fatal("exec failure: ", argv[1]);	/* None of them! */
		exit(127);
	}
	return wait(&s) == pid && s == 0;
}

static int check(char *path, register struct stat *st, register Node *n, char *last, int level) {
	if (n == NULL)
	  return 1;
	switch (n->n_type) {
		case OP_AND:
			return check(path, st, n->n_info.n_opnd.n_left, last, level) && 
				check(path, st, n->n_info.n_opnd.n_right, last, level);
		case OP_OR:
			return check(path, st, n->n_info.n_opnd.n_left, last, level) ||
				check(path, st, n->n_info.n_opnd.n_right, last, level);
		case NOT:
			return ! check(path, st, n->n_info.n_opnd.n_left, last, level);
		case OP_NAME:
			return sMatch(last, n->n_info.n_str);
		case OP_PERM:
			if (n->n_info.n_int.n_sign < 0)
			  return (st->st_mode & n->n_info.n_int.n_val) == n->n_info.n_int.n_val;
			return (st->st_mode & 07777) == n->n_info.n_int.n_val;
		case OP_NEWER:
			return st->st_mtime > n->n_info.n_int.n_val;
		case OP_TYPE:
			return (st->st_mode & S_IFMT) == (mode_t) n->n_info.n_int.n_val;
		case OP_LINKS:
			return ichk((long) st->st_nlink, n);
		case OP_USER:
			return st->st_uid == n->n_info.n_int.n_val;
		case OP_GROUP:
			return st->st_gid == n->n_info.n_int.n_val;
		case OP_SIZE:
			return ichk(st->st_size == 0 ? 0L : (long) ((st->st_size - 1) / BSIZE + 1), n);
		case OP_SIZEC:
			return ichk((long) st->st_size, n);
		case OP_INUM:
			return ichk((long) st->st_ino, n);
		case OP_ATIME:
			return ichk(st->st_atime, n);
		case OP_CTIME:
			return ichk(st->st_ctime, n);
		case OP_MTIME:
			return ichk(st->st_mtime, n);
		case OP_EXEC:
		case OP_OK:
			return execute(n->n_type, n->n_info.n_exec, path);
		case OP_PRINT:
			printf("%s\n", path);
			return 1;
		case OP_PRINT0:
			printf("%s", path);
			putchar('\0');
			return 1;
		case OP_XDEV:
		case OP_DEPTH:
			return 1;
		case OP_PRUNE:
			pruneHere = 1;
			return 1;
		case OP_NOUSER:
			return getpwuid(st->st_uid) == NULL;
		case OP_NOGROUP:
			return getgrgid(st->st_gid) == NULL;
		case OP_MAXDEPTH:
			return level <= n->n_info.n_int.n_val;
	}
	fatal("ILLEGAL NODE", "");
	return 0;	/* Never reached */
}

static void find(char *path, Node *pred, char *last, int level) {
	char sPath[PATH_MAX];
	register char *sEnd = sPath, *p;
	struct stat st;
	DIR *dp;
	struct dirent *entry;

	if (path[1] == '\0' && *path == '/') {
		*sEnd++ = '/';
		*sEnd = '\0';
	} else {
		while ((*sEnd++ = *path++)) {
		}
	}

	if (lstat(sPath, &st) == -1)
	  nonFatal("can't get status of ", sPath);
	else {
		switch (xdevFlag) {
			case 0:
				break;
			case 1:
				if (st.st_dev != devNum)
				  return;
				break;
			case 2:		/* Set current device number */
				xdevFlag = 1;
				devNum = st.st_dev;
				break;
		}

		pruneHere = 0;
		if (! depthFlag && check(sPath, &st, pred, last, level) && needPrint)
		  printf("%s\n", sPath);
		if (! pruneHere && S_ISDIR(st.st_mode)) {
			if ((dp = opendir(sPath)) == NULL) {
				nonFatal("can't read directory ", sPath);
				perror("Error");
				return;
			}
			sEnd[-1] = '/';
			while ((entry = readdir(dp)) != NULL) {
				p = entry->d_name;
				if (strcmp(p, ".") != 0 && strcmp(p, "..") != 0) {
					strcpy(sEnd, p);
					find(sPath, pred, sEnd, level + 1);
				}
			}
			closedir(dp);
		}
		if (depthFlag) {
			sEnd[-1] = '\0';
			if (check(sPath, &st, pred, last, level) && needPrint)
			  printf("%s\n", sPath);
		}
	}
}

void main(int argc, char **argv) {
	char **pathList, *path, *last;
	int pathCount = 0, i;
	Node *pred;

	prog = *argv++;
	if ((epath = getenv("PATH")) == NULL)
	  fatal("Can't get path from environment", "");
	umask(um = umask(0));	/* Non-destructive get-umask :-) */
	time(&currentTime);

	pathList = argv;
	while (--argc > 0 && lex(*argv) == NONE) {	/* find paths */
		++pathCount;
		++argv;
	}
	if (pathCount == 0)		/* There must be at least one path */
	  fatal("Usage: path-list [predicate-list]", "");

	ipp = argv;			/* Prepare for parsing */
	if (argc != 0) {	/* If there is anything to parse, */
		pred = expr(lex(*ipp));	/* then do so */
		if (lex(*++ipp) != EOI)	/* Make sure there's nothing left */
		  fatal("syntax error: garbage at end of predicate", "");
	} else {			/* No predicate list */
		pred = NULL;
	}

	for (i = 0; i < pathCount; ++i) {
		if (xdevFlag)
		  xdevFlag = 2;
		path = pathList[i];
		if ((last = strrchr(path, '/')) == NULL)
		  last = path;
		else
		  ++last;
		find(path, pred, last, 0);
	}
}



