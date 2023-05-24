
#include <unistd.h>
#include "shell.h"
#include "nodes.h"
#include "expand.h"
#include "syntax.h"
#include "options.h"
#include "mail.h"
#include "exec.h"
#include "eval.h"
#include "var.h"
#include "memalloc.h"
#include "output.h"
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

void shProcVar() {
	// no need
}

/* Search the environment of a builtin command. If the second argument
 * is nonzero, return the value of variable even if it hasn't been
 * exported.
 */
char *bltinLookup(char *name, int doAll) {
	StrList *sp;
	Var *v;

	for (sp = cmdEnviron; sp; sp = sp->next) {
		if (varEqual(sp->text, name))
		  return strchr(sp->text, '=') + 1;
	}
	for (v = *hashVar(name); v; v = v->next) {
		if (varEqual(v->text, name)) {
			if (v->flags & V_UNSET ||
					(! doAll && (v->flags & V_EXPORT) == 0))
			  return NULL;
			return strchr(v->text, '=') + 1;
		}
	}
	return NULL;
}

/* Find the value of a variable. Returns NULL if not set.
 */
char *lookupVar(char *name) {
	Var *v;

	for (v = *hashVar(name); v; v = v->next) {
		if (varEqual(v->text, name)) {
			if (v->flags & V_UNSET)
			  return NULL;
			return strchr(v->text, '=') + 1;
		}
	}
	return NULL;
}

/* Process a linked list of variable assignments.
 */
void listSetVar(struct StrList *list) {
	struct StrList *lp;

	INTOFF;
	for (lp = list; lp; lp = lp->next) {
		setVarEq(saveStr(lp->text), 0);
	}
	INTON;
}

/* Generate a list of exported variables. This routine is used to construct
 * the third argument to execve when executing a program.
 */
char **environment() {
	int nEnv;
	Var **vpp;
	Var *vp;
	char **env, **ep;

	nEnv = 0;
	for (vpp = varTable; vpp < varTable + VTAB_SIZE; ++vpp) {
		for (vp = *vpp; vp; vp = vp->next) {
			if (vp->flags & V_EXPORT)
			  ++nEnv;
		}
	}
	ep = env = stackAlloc((nEnv + 1) * sizeof(*env));
	for (vpp = varTable; vpp < varTable + VTAB_SIZE; ++vpp) {
		for (vp = *vpp; vp; vp = vp->next) {
			if (vp->flags & V_EXPORT)
			  *ep++ = vp->text;
		}
	}
	*ep = NULL;
	return env;
}

/* Command to list all variables which are set. Currently this command
 * is invoked from the set command when the set command is called without
 * any variables.
 */
int showVarsCmd(int argc, char **argv) {
	Var **vpp;
	Var *vp;

	for (vpp = varTable; vpp < varTable + VTAB_SIZE; ++vpp) {
		for (vp = *vpp; vp; vp = vp->next) {
			if ((vp->flags & V_UNSET) == 0)
			  out1Format("%s\n", vp->text);
		}
	}
	return 0;
}

/* Unset the specified variable. */
static void unsetVar(char *s) {
	Var **vpp;
	Var *vp;

	vpp = hashVar(s);
	for (vp = *vpp; vp; vpp = &vp->next, vp = *vpp) {
		if (varEqual(vp->text, s)) {
			INTOFF;
			if (*(strchr(vp->text, '=') + 1) != '\0' ||
					(vp->flags & V_READONLY)) {
				setVar(s, nullStr, 0);
			}
			vp->flags &= ~V_EXPORT;
			vp->flags |= V_UNSET;
			if ((vp->flags & V_STR_FIXED) == 0) {
				if ((vp->flags & V_TEXT_FIXED) == 0) 
				  ckFree(vp->text);
				*vpp = vp->next;
				ckFree(vp);
			}
			INTON;
			return;
		}
	}
}

/* The unset builtin command. We unset the function before we unset the
 * variable to allow a function to be unset when where is a readonly variable
 * with the same name.
 */
int unsetCmd(int argc, char **argv) {
	char **ap;

	for (ap = argv + 1; *ap; ++ap) {
		unsetFunc(*ap);
		unsetVar(*ap);
	}
	return 0;
}

int setVarCmd(int argc, char **argv) {
	if (argc <= 2) 
	  return unsetCmd(argc, argv);
	else if (argc == 3)
	  setVar(argv[1], argv[2], 0);
	else
	  error("List assignment not implemented");
	return 0;
}

/* The export and readonly commands. */
int exportCmd(int argc, char **argv) {
	Var **vpp;
	Var *vp;
	char *name;
	char *p;
	int flag = argv[0][0] == 'r' ? V_READONLY : V_EXPORT;

	listSetVar(cmdEnviron);
	if (argc > 1) {
		while ((name = *argPtr++) != NULL) {
			if ((p = strchr(name, '=')) != NULL) {	/* var=xxx */
				++p;
			} else {	/* value is null */
				vpp = hashVar(name);	
				for (vp = *vpp; vp; vp = vp->next) {
					if (varEqual(vp->text, name)) {
						vp->flags |= flag;
						goto found;
					}
				}
			}
			setVar(name, p, flag);
found:;
		}
	} else {
		for (vpp = varTable; vpp < varTable + VTAB_SIZE; ++vpp) {
			for (vp = *vpp; vp; vp = vp->next) {
				if (vp->flags & flag) {
					for (p = vp->text; *p != '='; ++p) {
						out1Char(*p);
					}
					out1Char('\n');
				}
			}
		}
	}
	return 0;
}

/* The "local" command. */
int localCmd(int argc, char **argv) {
	char *name;

	if (! isInFunction())
	  error("Not in a function");
	while ((name = *argPtr++) != NULL) {
		mkLocal(name);
	}
	return 0;
}

/* Make a variable a local variable. When a variable is made local, it's
 * value and flags are saved in a LocalVar structure. The saved values
 * will be restored when the shell function returns. We handle the name
 * "-" as a special case.
 */
void mkLocal(char *name) {
	LocalVar *lvp;
	Var **vpp;
	Var *vp;

	INTOFF;
	lvp = ckMalloc(sizeof(LocalVar));
	if (name[0] == '-' && name[1] == '\0') {
		lvp->text = ckMalloc(sizeof(optVal));
		bcopy(optVal, lvp->text, sizeof(optVal));	
		vp = NULL;
	} else {
		vpp = hashVar(name);
		for (vp = *vpp; vp && ! varEqual(vp->text, name); vp = vp->next) {
		}
		if (vp == NULL) {
			if (strchr(name, '='))
			  setVarEq(saveStr(name), V_STR_FIXED);
			else
			  setVar(name, NULL, V_STR_FIXED);
			vp = *vpp;	/* The new variable */
			lvp->text = NULL;
			lvp->flags = V_UNSET;
		} else {
			lvp->text = vp->text;
			lvp->flags = vp->flags;
			vp->flags |= V_STR_FIXED | V_TEXT_FIXED;
			if (strchr(name, '='))
			  setVarEq(saveStr(name), 0);
		}
	}
	lvp->vp = vp;
	lvp->next = localVars;
	localVars = lvp;
	INTON;
}

/* Called after a function returns.
 */
void popLocalVars() {
	LocalVar *lvp;
	Var *vp;

	while ((lvp = localVars) != NULL) {
		localVars = lvp->next;
		vp = lvp->vp;
		if (vp == NULL) {	/* $- saved */
			bcopy(lvp->text, optVal, sizeof(optVal));
			ckFree(lvp->text);
		} else if ((lvp->flags & (V_UNSET | V_STR_FIXED)) == V_UNSET) {
			unsetVar(vp->text);
		} else {
			if ((vp->flags & V_TEXT_FIXED) == 0)
			  ckFree(vp->text);
			vp->flags = lvp->flags;
			vp->text = lvp->text;
		}
		ckFree(lvp);
	}
}


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

MKINIT void shProcVar();

SHELLPROC {
	shProcVar();
}
#endif






