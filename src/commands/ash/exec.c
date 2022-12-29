

#include "shell.h"
#include "nodes.h"
#include "mail.h"
#include "parser.h"
#include "exec.h"
#include "var.h"
#include "error.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "errno.h"
#include "mystring.h"
#include "memalloc.h"
#include "builtins.h"
#include "limits.h"


#define CMD_TABLE_SIZE	31		/* Should be prime */
#define ARB	1					/* Actual size determined at run time */


typedef struct TableEntry {
	struct TableEntry *next;	/* Next entry in hash chain */
	union Param param;			/* Definition of builtin function */
	short cmdType;				/* Index identifying command */
	char rehash;				/* If set, cd done since entry created */
	char cmdName[ARB];			/* Name of command */
} TableEntry;

static TableEntry *cmdTable[CMD_TABLE_SIZE];
static int builtinLoc = -1;		/* Index in path of %builtin, or -1 */


/* Clear out command entries. The argument specifies the first entry in
 * PATH which has changed.
 */
static void clearCmdEntry(int firstChange) {
	TableEntry **tblp, **pp, *cmdp;

	INTOFF;
	for (tblp = cmdTable; tblp < &cmdTable[CMD_TABLE_SIZE]; ++tblp) {
		pp = tblp;
		while ((cmdp = *pp) != NULL) {
			if ((cmdp->cmdType == CMD_NORMAL && cmdp->param.index >= firstChange) ||
					(cmdp->cmdType == CMD_BUILTIN && builtinLoc >= firstChange)) {
				*pp = cmdp->next;
				ckFree(cmdp);
			} else {
				pp = &cmdp->next;
			}
		}
	}
	INTON;
}


/* Called before PATH is changed. The argument is the new value of PATH;
 * pathVal() still returns the old value at this point. Called with
 * interrupts off.
 */
void changePath(char *newVal) {
	char *old, *new;
	int index;
	int firstChange;
	int bltin;

	old = pathVal();
	new = newVal;
	firstChange = 9999;	/* Assume no change */
	index = 0;
	bltin = -1;
	for (;;) {
		if (*old != *new) {
			firstChange = index;
			if ((*old == '\0' && *new == ':') ||
				(*old == ':' && *new == '\0'))
			  ++firstChange;
			old = new;		/* Ignore subsequent differences */
		}
		if (*new == '\0')
		  break;
		if (*new == '%' && bltin < 0 && prefix("builtin", new + 1))
		  bltin = index;
		if (*new == ':')
		  ++index;
		++new;
		++old;
	}
	if (builtinLoc < 0 && bltin >= 0)
	  builtinLoc = bltin;
	if (builtinLoc >= 0 && bltin < 0)
	  firstChange = 0;
	clearCmdEntry(firstChange);
	builtinLoc = bltin;
}

void deleteFuncs() {
	//TODO
}

/* Locate a command in the command hash table. If "add" is nonzero,
 * add the command to the table if it is not already present. The
 * variable "lastCmdEntry" is set to point to the address of the link
 * pointing to the entry, so that deleteCmdEntry can delete the entry.
 */
TableEntry **lastCmdEntry;

static TableEntry *cmdLookup(char *name, int add) {
	int hashVal;
	register char *p;
	TableEntry *cmdp;
	TableEntry **pp;

	p = name;
	hashVal = *p << 4;
	while (*p) {
		hashVal += *p++;
	}
	hashVal &= 0x7FFF;
	pp = &cmdTable[hashVal % CMD_TABLE_SIZE];
	for (cmdp = *pp; cmdp; cmdp = cmdp->next) {
		if (equal(cmdp->cmdName, name))
		  break;
		pp = &cmdp->next;
	}
	if (add && cmdp == NULL) {
		INTOFF;
		cmdp = *pp = ckMalloc(sizeof(TableEntry) - ARB + strlen(name) + 1);
		cmdp->next = NULL;
		cmdp->cmdType = CMD_UNKNOWN;
		cmdp->rehash = 0;
		strcpy(cmdp->cmdName, name);
		INTON;
	}
	lastCmdEntry = pp;
	return cmdp;
}

/* Resolve a command name. If you change this routine, you may have to 
 * change the shellExec routine as well.
 */
void findCommand(char *name, CmdEntry *entry, int printErr) {
	TableEntry *cmdp;
	/*
	int index;
	int prev;
	char *path;
	char *fullName;
	struct stat sb;
	int e;
	*/
	int i;

	/* If name contains a slash, don't use the hash table */
	if (strchr(name, '/') != NULL) {
		entry->cmdType = CMD_NORMAL;
		entry->u.index = 0;
		return;
	}

	/* If name is in the table, and not invalidated by cd, we're done */
	if ((cmdp = cmdLookup(name, 0)) != NULL && cmdp->rehash == 0)
	  goto success;

	/* If %builtin not in path, check for builtin next */
	if (builtinLoc < 0 && (i = findBuiltin(name)) >= 0) {
		INTOFF;
		cmdp = cmdLookup(name, 1);
		cmdp->cmdType = CMD_BUILTIN;
		cmdp->param.index = i;
		INTON;
		goto success;
	}

//TODO 


success:
	cmdp->rehash = 0;
	entry->cmdType = cmdp->cmdType;
	entry->u = cmdp->param;
}

/* Search the table of builtin commands */
int findBuiltin(char *name) {
	const register BuiltinCmd *bp;

	for (bp = builtinCmd; bp->name; ++bp) {
		if (*bp->name == *name && equal(bp->name, name))
		  return bp->code;
	}
	return -1;
}

/* Exec a program. Never returns. If you change this rontine, you may
 * have to change the findCommand routine as well.
 */
void shellExec(char **argv, char **envp, char *path, int index) {
	//TODO
}

/* Delete the command entry returned on the last lookup. */
static void deleteCmdEntry() {
	TableEntry *cmdp;

	INTOFF;
	cmdp = *lastCmdEntry;
	*lastCmdEntry = cmdp->next;
	ckFree(cmdp);
	INTON;
}

/* Delete a function if it exists. */
void unsetFunc(char *name) {
	TableEntry *cmdp;

	if ((cmdp = cmdLookup(name, 0)) != NULL && cmdp->cmdType == CMD_FUNCTION) {
		freeFunc(cmdp->param.func);
		deleteCmdEntry();
	}
}

/* Command hashing ode */
int hashCmd(int argc, char **argv) {
	printf("=== hashCmd\n");//TODO
	return 0;
}

/* Add a new command entry, replacing any existing command entry for
 * the same name.
 */
static void addCmdEntry(char *name, CmdEntry *entry) {
	TableEntry *cmdp;

	INTOFF;
	cmdp = cmdLookup(name, 1);
	if (cmdp->cmdType == CMD_FUNCTION) 
	  freeFunc(cmdp->param.func);
	cmdp->cmdType = entry->cmdType;
	cmdp->param = entry->u;
	INTON;
}

/* Define a shell function. */
void defineFunc(char *name, union Node *func) {
	CmdEntry entry;

	INTOFF;
	entry.cmdType = CMD_FUNCTION;
	entry.u.func = copyFunc(func);
	addCmdEntry(name, &entry);
	INTON;
}


#ifdef mkinit
MKINIT void deleteFuncs();

SHELLPROC {
	deleteFuncs();
}
#endif
