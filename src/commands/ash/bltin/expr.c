/*
 * The expr and test commands.
 */

#define main	exprCmd

#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "stdlib.h"
#include "bltin.h"
#include "operators.h"

#define STACK_SIZE	12
#define NEST_INCR	16

/* Data types */
#define STRING	0
#define INTEGER	1
#define BOOLEAN 2


/* This structure hold a value. Tye type keyword specifies the type of
 * the value, and the union u holds the value. The value of a boolean
 * is stored in u.num (1 = TRUE, 0 = FALSE).
 */
typedef struct {
	int type;
	union {
		char *string;
		long num;
	} u;
} Value;

typedef struct {
	short op;		/* Which operator */
	short pri;		/* Priority of operator */
} Operator;

typedef struct {
	int op;			/* OP_FILE or OP_LFILE */
	char *name;		/* Name of file */
	int retCode;	/* Return code from stat */
	struct stat stat;	/* Status info on file */
} FileStat;

extern char *matchBegin[10];	/* Matched string */
extern short matchLength[10];	/* Defined in regexp.c */
extern short numberParens;		/* Number of \( \) pairs */


static int lookupOp(char *name, char *const *table) {
	register char *const *tp;
	register char const *p;
	char c = name[1];

	for (tp = table; (p = *tp) != NULL; ++tp) {
		if (p[1] == c && equal(p, name))
		  return tp - table;
	}
	return -1;
}

static int exprIsFalse(Value *val) {	
	if (val->type == STRING) {
		if (val->u.string[0] == '\0')
		  return 1;
	} else {	/* INTEGER or BOOLEAN */
		if (val->u.num == 0)
		  return 1;
	}
	return 0;
}

/* Execute an operator. Op is the operator. Sp is the stack pointer;
 * sp[0] refers to the first operand, sp[1] refers to the second operand
 * (if any), and the result is placed in sp[0]. The operands are converted
 * to the type expected by the operator before exprOperator is called.
 * Fs is a pointer to a structure which holds the value of the last call
 * to stat, to avoid repeated stat calls on the same file.
 */
static void exprOperator(int op, Value *sp, FileStat *fs) {
	int i;
	struct stat st1, st2;

	switch (op) {
		case NOT:
			sp->u.num = exprIsFalse(sp);
			sp->type = BOOLEAN;
			break;
		case IS_READ:
			i = 04;
			goto permission;
		case IS_WRITE:
			i = 02;
			goto permission;
		case IS_EXEC:
			i = 01;
permission:
			if (fs->stat.st_uid == geteuid())
			  i <<= 6;
			else if (fs->stat.st_gid == getegid())
			  i <<= 3;
			goto fileBit;	/* True if (stat.st_mode & i) != 0 */
		case IS_FILE:
			i = S_IFREG;
			goto fileType;
		case IS_DIR:
			i = S_IFDIR;
			goto fileType; 
		case IS_CHAR: 
			i = S_IFCHR;
			goto fileType;
		case IS_BLOCK:
			i = S_IFBLK;
			goto fileType;
		case IS_FIFO:
			i = S_IFIFO;
			goto fileType;
fileType:
			if ((fs->stat.st_mode & S_IFMT) == i && fs->retCode >= 0) { 
true:
				sp->u.num = 1;
			} else {
false:
				sp->u.num = 0;
			}
			sp->type = BOOLEAN;
			break;
		case IS_SETUID:
			i = S_ISUID;
			goto fileBit;
		case IS_SETGID:
			i = S_ISGID;
			goto fileBit;
		case IS_STICKY:
			i = S_ISVTX;
fileBit:
			if (fs->stat.st_mode & i && fs->retCode >= 0)
			  goto true;
			goto false;
		case IS_SIZE:
			sp->u.num = fs->retCode >= 0 ? fs->stat.st_size : 0L;
			sp->type = INTEGER;
			break;
		case IS_LINK1:
		case IS_LINK2:
			if (S_ISLNK(fs->stat.st_mode) && fs->retCode >= 0)
			  goto true;
			fs->op = OP_FILE;	/* Not a symlink, so expect a -d or so next */
			goto false;
		case NEWER:
			if (stat(sp->u.string, &st1) != 0) 
			  sp->u.num = 0;
			else if (stat((sp + 1)->u.string, &st2) != 0)
			  sp->u.num = 1;
			else
			  sp->u.num = st1.st_mtime >= st2.st_mtime;
			sp->type = INTEGER;
			break;
		case IS_TTY:
			sp->u.num = isatty(sp->u.num);
			sp->type = BOOLEAN;
			break;
		case NUL_STR:
			if (sp->u.string[0] == '\0')
			  goto true;
			goto false;
		case STR_LEN:
			sp->u.num = strlen(sp->u.string);
			sp->type = INTEGER;
			break;
		case OR1:
		case AND1:
			/* These operators are mostly handled by the parser. If we
			 * get here it means that both operands were evaluated, so
			 * the value is the value of the second operand.
			 */
			*sp = *(sp + 1);
			break;
		case STR_EQ:
		case STR_NE:
			i = 0;
			if (equal(sp->u.string, (sp + 1)->u.string))
			  ++i;
			if (op == STR_NE)
			  i = 1 - i;
			sp->u.num = i;
			sp->type = BOOLEAN;
			break;
		case EQ:
			if (sp->u.num == (sp + 1)->u.num)
			  goto true;
			goto false;
		case NE:
			if (sp->u.num != (sp + 1)->u.num)
			  goto true;
			goto false;
		case GT:
			if (sp->u.num > (sp + 1)->u.num)
			  goto true;
			goto false;
		case LT:
			if (sp->u.num < (sp + 1)->u.num)
			  goto true;
			goto false;
		case LE:
			if (sp->u.num <= (sp + 1)->u.num)
			  goto true;
			goto false;
		case GE:
			if (sp->u.num >= (sp + 1)->u.num)
			  goto true;
			goto false;
		case PLUS:
			sp->u.num += (sp + 1)->u.num;
			break;
		case MINUS:
			sp->u.num -= (sp + 1)->u.num;
			break;
		case TIMES:
			sp->u.num *= (sp + 1)->u.num;
			break;
		case DIVIDE:
			if ((sp + 1)->u.num == 0)
			  error("Division by zero");
			sp->u.num /= (sp + 1)->u.num;
			break;
		case REM:
			if ((sp + 1)->u.num == 0)
			  error("Division by zero");
			sp->u.num %= (sp + 1)->u.num;
			break;
		case MATCH_PAT:
			{
				//TODO
			}
			break;

	}
}

int main(int argc, char **argv) {
	char **ap;
	char *opName;
	char c;
	char *p;
	int print;
	int nest;		/* Parentheses nesting */
	int op;
	int pri;
	int skipping;
	int binary;
	Operator opStack[STACK_SIZE];
	Operator *opSp;
	Value valStack[STACK_SIZE + 1];
	Value *valSp;
	FileStat fs;

	INIT_ARGS(argv);
	c = **argv;
	print = 1;
	if (c == 't')
	  print = 0;
	else if (c == '[') {
		if (! equal(argv[argc - 1], "]")) 
		  error("missing ]");
		argv[argc - 1] = NULL;
		print = 0;
	}
	ap = argv + 1;
	fs.name = NULL;

	/* We use operator precedence parsing, evaluating the expression
	 * as we parse it. Parentheses are handled by bumping up the 
	 * priority of operators using the variable "nest". We use the
	 * variable "skipping" to turn off evaluation temporarily for the
	 * short circuit boolean operators. (It is important do the short
	 * circuit evaluation because under NFS a stat operation can take
	 * infinitely long.)
	 */
	nest = 0;
	skipping = 0;
	opSp = opStack + STACK_SIZE;
	valSp = valStack;
	if (*ap == NULL) {
		valStack[0].type = BOOLEAN;
		valStack[0].u.num = 0;
		goto done;
	}
	for (;;) {
		opName = *ap++;
		if (opName == NULL)
syntax:   error("syntax error");
		if (opName[0] == '(' && opName[1] == '\0') {	/* '(', enter (...) */	
			nest += NEST_INCR;
			continue;
		} else if (*ap && (op = lookupOp(opName, unaryOp)) >= 0) {	/* Unary op */
			if (opSp == &opStack[0])
overflow:     error("Expression too complex");
			--opSp;		/* Push unary op down on the op stack */
			opSp->op = op;
			opSp->pri = opPriority[op] + nest;
			continue;	/* Continue to get the following operand */
		} else {	/* Binary operand, push it up on the value stack */
			valSp->type = STRING;
			valSp->u.string = opName;
			++valSp;
		}
		for (;;) {
			opName = *ap++;
			if (opName == NULL) {
				if (nest != 0)
				  goto syntax;
				pri = 0;
				break;
			}
			if (opName[0] != ')' || opName[1] != '\0') {	/* Binary op */
				if ((op = lookupOp(opName, binaryOp)) < 0)
				  goto syntax;
				op += FIRST_BINARY_OP;
				pri = opPriority[op] + nest;	/* If we are in (...), need to raise priority */
				break;
			}
			if ((nest -= NEST_INCR) < 0)	/* ')', leave (...) */
			  goto syntax;
		}
		while (opSp < &opStack[STACK_SIZE] &&	/* We need to calculate. */
					opSp->pri >= pri) {	/* opSp may be in (...) or has a higher operator priority than current pri */
			/* Convert operands */
			binary = opSp->op;
			for (;;) {
				--valSp;
				c = opArgFlag[opSp->op];
				if (c == OP_INT) {	/* Convert value to Integer */
					if (valSp->type == STRING)
					  valSp->u.num = atol(valSp->u.string);
					valSp->type = INTEGER;
				} else if (c >= OP_STRING) {	/* OP_STRING or OP_FILE */
					if (valSp->type == INTEGER) {	/* Convert int to string */
						p = stackAlloc(32);
#ifdef SHELL
						formatStr(p, 32, "%d", valSp->u.num);
#else
						sprintf(p, "%d", valSp->u.num);
#endif
						valSp->u.string = p;
					} else if (valSp->type == BOOLEAN) {	/* Convert bool to string */
						if (valSp->u.num)
						  valSp->u.string = "true";
						else
						  valSp->u.string = "";
					}
					valSp->type = STRING;
					/* Get file stat */
					if (c >= OP_FILE &&
							(fs.op != c ||
							 fs.name == NULL ||
							 ! equal(fs.name, valSp->u.string))) {
						fs.op = c;
						fs.name = valSp->u.string;
						if (c == OP_FILE) 
						  fs.retCode = stat(valSp->u.string, &fs.stat);
						else
						  fs.retCode = lstat(valSp->u.string, &fs.stat);
					}
				} else {
					/* For NOT, AND, OR, no need to convert the operand */
				}
				if (binary < FIRST_BINARY_OP)	/* If opSp is unary, only convert the first value */
				  break;
				binary = 0;		/* Else, convert the second value */
			}
			if (! skipping) 
			  exprOperator(opSp->op, valSp, &fs);	/* Calculate and push the result value on the stack */
			else if (opSp->op == AND1 || opSp->op == OR1) 
			  --skipping;
			++valSp;	
			++opSp;		/* Remove the used operator */
		}
		if (opName == NULL)
		  break;
		if (opSp == &opStack[0])
		  goto overflow;
		if (op == AND1 || op == AND2) {
			op = AND1;
			/* If the first branch is false, no need to check the second */
			if (skipping || exprIsFalse(valSp - 1))	
			  ++skipping;
		}
		if (op == OR1 || op == OR2) {
			op = OR1;
			/* If the first branch is true, no need to check the second */
			if (skipping || ! exprIsFalse(valSp - 1))
			  ++skipping;
		}
		/* Push the current op down on the op stack */
		--opSp;
		opSp->op = op;
		opSp->pri = pri;
	}
done:
	if (print) {
		if (valStack[0].type == STRING)
		  printf("%s\n", valStack[0].u.string);
		else if (valStack[0].type == INTEGER)
		  printf("%ld\n", valStack[0].u.num);
		else if (valStack[0].u.num != 0)
		  printf("true\n");
	}
	return exprIsFalse(&valStack[0]); 
}

