
#include "unistd.h"
#include "shell.h"
#include "syntax.h"
#include "mail.h"
#include "exec.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"

#define VTAB_SIZE	39

typedef struct {
	Var *var;
	int flags;
	char *text;
} VarInit;

#if ATTY
Var vatty;
#endif
Var vifs;
Var vmail;
Var vmpath; 
Var vpath;
Var vps1;
Var vps2;
Var vpse;
#if ATTY
Var vterm;
#endif

const VarInit varInit[] = {
#if ATTY
	{&vatty,    V_STR_FIXED | V_TEXT_FIXED | V_UNSET, "ATTY="},
#endif
	{&vifs,     V_STR_FIXED | V_TEXT_FIXED,           "IFS= \t\n"},
	{&vmail,    V_STR_FIXED | V_TEXT_FIXED | V_UNSET, "MAIL="},
	{&vmpath,   V_STR_FIXED | V_TEXT_FIXED | V_UNSET, "MAILPATH="},
	{&vpath,    V_STR_FIXED | V_TEXT_FIXED | V_UNSET, "PATH=:/bin:/usr/bin"},
	/* vps1 depends on uid */
	{&vps2,     V_STR_FIXED | V_TEXT_FIXED,           "PS2=> "},
	{&vpse,     V_STR_FIXED | V_TEXT_FIXED,           "PSE=* "},
#if ATTY
	{&vterm,    V_STR_FIXED | V_TEXT_FIXED | V_UNSET, "TERM="},
#endif
	{NULL, 0, NULL}
};

Var *varTable[VTAB_SIZE];


/* Initialize the variable symbol tables and import the environment.
 */
#ifdef mkinit
INCLUDE "var.h"

INIT {
	char **envp;
	extern char **environ;

	initVar();
	for (envp = environ; *envp; ++envp) {
		if (strchr(*envp, '=')) {
			setVarEq(*envp, V_EXPORT | V_TEXT_FIXED);
		}
	}
}
#endif


/* Find the appropriate entry in the hash table from the name.
 */
static Var **hashVar(register char *p) {
	unsigned int hashVal;

	hashVal = *p << 4;
	while (*p && *p != '=') {
		hashVal += *p++;
	}
	return &varTable[hashVal % VTAB_SIZE];
}


/* This routine initializes the builtin variables. It is called when the
 * shell is initialized and again when a shell procedure is spawned.
 */

void initVar() {
	const VarInit *ip;
	Var *vp, **vpp;

	for (ip = varInit; (vp = ip->var) != NULL; ++ip) {
		if ((vp->flags & V_EXPORT) == 0) {
			vpp = hashVar(ip->text);
			vp->next = *vpp;
			*vpp = vp;
			vp->text = ip->text;
			vp->flags = ip->flags;
		}
	}
	/* PS1 depends on uid
	 */
	if ((vps1.flags & V_EXPORT) == 0) {
		vpp = hashVar("PS1=");
		vps1.next = *vpp;
		*vpp = &vps1;
		vps1.text = getuid() ? "PS1=$ " : "PS1=# ";
		vps1.flags = V_STR_FIXED | V_TEXT_FIXED;
	}
}

/* Set the value of a variable. The flags argument is ored with the
 * flags of the variable. If val is NULL, the variable is unset.
 */
void setVar(char *name, char *val, int flags) {
	char *p, *q, *nameEq;
	int len, nameLen;
	int isBad;

	isBad = 0;
	p = name;
	if (! isName((int) *p++))
	  isBad = 1;
	for (;;) {
		if (! isInName((int) *p)) {
			if (*p == 0 || *p == '=')
			  break;
			isBad = 1;
		}
		++p;
	}
	nameLen = p - name;
	if (isBad)
	  error("%.*s: is read only", nameLen, name);
	len = nameLen + 2;		/* 2 is space for '=' and '\0' */
	if (val == NULL)
	  flags |= V_UNSET;
	else
	  len += strlen(val);
	p = nameEq = ckMalloc(len);
	q = name;
	while (--nameLen >= 0) {
		*p++ = *q++;
	}
	*p++ = '=';
	*p = 0;
	if (val)
	  scopy(val, p);
	setVarEq(nameEq, flags);
}

/* Returns true if the two strings specify the same variable. The
 * first variable name is terminated by '='; the second may be 
 * terminated by either '=' or '\0'.
 */
static int varEqual(register char *p, register char *q) {
	while (*p == *q++) {
		if (*p++ == '=')
		  return 1;
	}
	if (*p == '=' && *(q - 1) == '\0')
	  return 1;
	return 0;
}

/* Same as setVar except that the variable and value are passed in
 * the first argument as name=value. Since the first argument will
 * be actually stored in the table, it should not be a string that
 * will go away.
 */
void setVarEq(char *s, int flags) {
	Var *vp, **vpp;

	vpp = hashVar(s);
	for (vp = *vpp; vp; vp = vp->next) {
		if (varEqual(s, vp->text)) {
			if (vp->flags & V_READONLY) {
				int len = strchr(s, '=') - s;
				error("%.*s: is read only", len, s);
			}
			INTOFF;
			if (vp == &vpath)
			  changePath(s + 5);	/* 5  strlen("PATH=") */
			if ((vp->flags & (V_TEXT_FIXED | V_STACK)) == 0)
			  ckFree(vp->text);
			vp->flags &= ~(V_TEXT_FIXED | V_STACK | V_UNSET);
			vp->flags |= flags;
			vp->text = s;
			if (vp == &vmpath || (vp == &vmail && ! mpathSet()))
			  checkMail(1);
			INTON;
			return;
		}
	}
	/* Not found */
	vp = ckMalloc(sizeof(*vp));
	vp->flags = flags;
	vp->text = s;
	vp->next = *vpp;
	*vpp = vp;
}








