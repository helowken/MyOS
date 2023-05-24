


#include "shell.h"
#include "nodes.h"
#include "syntax.h"
#include "expand.h"
#include "parser.h"
#include "jobs.h"
#include "eval.h"
#include "exec.h"
#include "builtins.h"
#include "input.h"
#include "output.h"
#include "trap.h"
#include "var.h"
#include "options.h"
#include "redir.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#include <signal.h>
#include <unistd.h>


/* Flags in argument to evalTree */
#define EV_EXIT		01	/* Exit after evaluating tree */
#define EV_TESTED	02	/* Exit status is checked; ignore -e flag */
#define EV_BACKCMD	04	/* Command executing within back quotes */

/* Reasons for skipping commands (see comment on breakCmd routine) */
#define SKIP_BREAK	1
#define SKIP_CONT	2
#define SKIP_FUNC	3


MKINIT int evalSkip;	/* Set if we are skipping commands */
static int skipCount;	/* Number of levels to skip */
MKINIT int loopNest;	/* Current loop nesting level */
int funcNest;			/* Depth of function calls */

char *commandName;
StrList *cmdEnviron;
int exitStatus;			/* Exit status of last command */


/* Called to reset things after an exception.
 */
#ifdef mkinit
INCLUDE "eval.h"

RESET {
	evalSkip = 0;
	loopNest = 0;
	funcNest = 0;
}

SHELLPROC {
	exitStatus = 0;
}
#endif



/* Execute a command or commands contained in a string.
 */
void evalString(char *s) {
	Node *n;
	StackMark stackMark;

	setStackMark(&stackMark);
	setInputString(s, 1);
	while ((n = parseCmd(0)) != NEOF) {
		evalTree(n, 0);
		popStackMark(&stackMark);
	}
	popFile();
	popStackMark(&stackMark);
}

/* Compute the names of the files in a redirection list. 
 */
static void expRedir(Node *n) {
	register Node *redir;

	for (redir = n; redir; redir = redir->nFile.next) {
		if (redir->type == NFROM ||
				redir->type == NTO ||
				redir->type == NAPPEND) {
			ArgList fn;
			fn.last = &fn.list;
			expandArg(redir->nFile.fileName, &fn, 0);
			redir->nFile.expFileName = fn.list->text;
		}
	}
}

static void evalCommand(Node *cmd, int flags, BackCmd *backCmd) {
	StackMark stackMark;
	Node *argp;
	ArgList argList;
	ArgList varList;
	char **argv;
	int argc;
	char **envp;
	int varFlag;
	StrList *sp;
	register char *p;
	int mode;
	int pip[2];
	CmdEntry cmdEntry;
	Job *jp;
	JmpLoc jmpLoc;
	JmpLoc *volatile saveHandler;
	char *volatile saveCmdName;
	volatile ShellParam saveParam;
	LocalVar *volatile saveLocalVars;
	volatile int e;
	char *lastArg;

	/* First expand the arguments. */
	setStackMark(&stackMark);
	argList.last = &argList.list;
	varList.last = &varList.list;
	varFlag = 1;
	for (argp = cmd->nCmd.args; argp; argp = argp->nArg.next) {
		p = argp->nArg.text;
		if (varFlag && isName(*p)) {	/* expand xx=yy */
			do {
				++p;
			} while (isInName(*p));
			if (*p == '=') {
				expandArg(argp, &varList, 0);
				continue;
			}
		}
		expandArg(argp, &argList, 1);	/* expand args */
		varFlag = 0;
	}
	*argList.last = NULL;
	*varList.last = NULL;
	expRedir(cmd->nCmd.redirect);

	/* Compute argc, populate argv and set lastArg if needed. */
	argc = 0;
	for (sp = argList.list; sp; sp = sp->next) {
		++argc;
	}
	argv = stackAlloc(sizeof (char *) * (argc + 1));
	for (sp = argList.list; sp; sp = sp->next) {
		*argv++ = sp->text;
	}
	*argv = NULL;
	lastArg = NULL;
	if (iflag && funcNest == 0 && argc > 0) {
		lastArg = argv[-1];
	}
	argv -= argc;

	/* Print the command if xFlag is set. */
	if (xflag == 1) {
		outChar('+', &errOut);
		for (sp = varList.list; sp; sp = sp->next) {
			outChar(' ', &errOut);
			out2Str(sp->text);
		}
		for (sp = argList.list; sp; sp = sp->next) {
			outChar(' ', &errOut);
			out2Str(sp->text);
		}
		outChar('\n', &errOut);
		flushOut(&errOut);
	}

	/* Now locate the command. */
	if (argc == 0) {
		cmdEntry.cmdType = CMD_BUILTIN;
		cmdEntry.u.index = BLTINCMD;
	} else {
		findCommand(argv[0], &cmdEntry, 1);
		if (cmdEntry.cmdType == CMD_UNKNOWN) {	/* Command not found */
			exitStatus = 2;
			flushOut(&errOut);
			popStackMark(&stackMark);
			return;
		}
		/* Implement the bltin builtin here */
		if (cmdEntry.cmdType == CMD_BUILTIN && cmdEntry.u.index == BLTINCMD) {
			for (;;) {
				++argv;
				if (--argc == 0)
				  break;
				if ((cmdEntry.u.index = findBuiltin(*argv)) < 0) {
					outFormat(&errOut, "%s: not found\n", *argv);
					exitStatus = 2;
					flushOut(&errOut);
					popStackMark(&stackMark);
					return;
				}
				if (cmdEntry.u.index != BLTINCMD)
				  break;
			}
		}
	}

	/* Fork off a child process if necessary. */
	if (cmd->nCmd.backgnd ||
			(cmdEntry.cmdType == CMD_NORMAL && (flags & EV_EXIT) == 0) ||
			((flags & EV_BACKCMD) != 0 &&
				(cmdEntry.cmdType != CMD_BUILTIN ||
					cmdEntry.u.index == DOTCMD ||
					cmdEntry.u.index == EVALCMD))) {
		jp = makeJob(1);
		mode = cmd->nCmd.backgnd;
		if (flags & EV_BACKCMD) {
			mode = FORK_NO_JOB;
			if (pipe(pip) < 0) 
			  error("Pipe call failed");
		}
		if (forkShell(jp, cmd, mode) != 0)
		  goto parent;		/* At end of routine */
		if (flags & EV_BACKCMD) {
			FORCE_INTON;
			close(pip[0]);	/* Child proc close the read point */
			copyToStdout(pip[1]);	/* Child proc copy the write point to stdout. */
		}
		flags |= EV_EXIT;
	}

	/* This is the child process if a fork occurred. */
	/* Execute the command. */
	if (cmdEntry.cmdType == CMD_FUNCTION) {
		redirect(cmd->nCmd.redirect, REDIR_PUSH);
		saveParam = shellParam;
		shellParam.malloc = 0;
		shellParam.numParam = argc - 1;
		shellParam.params = argv + 1;
		shellParam.optNext = NULL;
		INTOFF;
		saveLocalVars = localVars;
		localVars = NULL;
		INTON;
		if (setjmp(jmpLoc.loc)) {
			if (exception == EX_SHELL_PROC)
			  freeParam((ShellParam *) &saveParam);
			else {
				freeParam(&shellParam);
				shellParam = saveParam;
			}
			popLocalVars();
			localVars = saveLocalVars;
			handler = saveHandler;
			longjmp(handler->loc, 1);
		}
		saveHandler = handler;
		handler = &jmpLoc;
		for (sp = varList.list; sp; sp = sp->next) {
			mkLocal(sp->text);
		}
		++funcNest;
		evalTree(cmdEntry.u.func, 0);
		--funcNest;
		INTOFF;
		popLocalVars();
		localVars = saveLocalVars;
		freeParam(&shellParam);
		shellParam = saveParam;
		handler = saveHandler;
		popRedir();
		INTON;
		if (evalSkip == SKIP_FUNC) {
			evalSkip = 0;
			skipCount = 0;
		}
		if (flags & EV_EXIT)
		  exitShell(exitStatus);
	} else if (cmdEntry.cmdType == CMD_BUILTIN) {
		mode = (cmdEntry.u.index == EXECCMD) ? 0 : REDIR_PUSH;
		if (flags == EV_BACKCMD) {
			memOut.numLeft = 0;
			memOut.nextChar = memOut.buf;
			memOut.bufSize = 64;
			mode |= REDIR_BACK_Q;
		}
		redirect(cmd->nCmd.redirect, mode);
		saveCmdName = commandName;
		cmdEnviron = varList.list;
		e = -1;
		if (setjmp(jmpLoc.loc)) {
			e = exception;
			exitStatus = (e == EX_INT) ? SIGINT + 128 : 2;
			goto cmdDone;
		}
		saveHandler = handler;
		handler = &jmpLoc;
		commandName = argv[0];
		argPtr = argv + 1;
		optPtr = NULL;		/* Initialize nextOpt */
		exitStatus = (*builtinFunc[cmdEntry.u.index])(argc, argv);
		flushAll();
cmdDone:
		out1 = &output;
		out2 = &errOut;
		freeStdout();
		if (e != EX_SHELL_PROC) {
			commandName = saveCmdName;
			if (flags & EV_EXIT)
			  exitShell(exitStatus);
		}
		handler = saveHandler;
		if (e != -1) {
			if (e != EX_ERROR ||
					cmdEntry.u.index == BLTINCMD ||
					cmdEntry.u.index == DOTCMD ||
					cmdEntry.u.index == EVALCMD ||
					cmdEntry.u.index == EXECCMD)
			  exRaise(e);
			FORCE_INTON;
		}
		if (cmdEntry.u.index != EXECCMD)
		  popRedir();
		if (flags == EV_BACKCMD) {
			backCmd->buf = memOut.buf;
			backCmd->numLeft = memOut.nextChar - memOut.buf;
			memOut.buf = NULL;
		}
	} else {
		clearRedir();
		redirect(cmd->nCmd.redirect, 0);
		if (varList.list) {
			p = stackAlloc(strlen(pathVal()) + 1);
			scopy(pathVal(), p);
		} else {
			p = pathVal();
		}
		for (sp = varList.list; sp; sp = sp->next) {
			setVarEq(sp->text, V_EXPORT | V_STACK);
		}
		envp = environment();
		shellExec(argv, envp, p, cmdEntry.u.index);
		/* Not reached. */
	}
	goto out;

parent:		/* Parent process gets here (if we forked) */
	if (mode == FORK_FG) {	/* Argument to fork */
		INTOFF;
		exitStatus = waitForJob(jp);
		INTON;
	} else if (mode == FORK_NO_JOB) {
		backCmd->fd = pip[0];
		close(pip[1]);
		backCmd->jp = jp;
	}

out:
	if (lastArg)
	  setVar("_", lastArg, 0);
	popStackMark(&stackMark);
}

static void evalLoop(Node *n) {
	int status;

	++loopNest;
	status = 0;
	for (;;) {
		evalTree(n->nBinary.ch1, EV_TESTED);
		if (evalSkip) {
skipping:	if (evalSkip == SKIP_CONT && --skipCount <= 0) {
				evalSkip = 0;
				continue;
			}
			if (evalSkip == SKIP_BREAK && --skipCount <= 0)
			  evalSkip = 0;
			break;
		}
		if (n->type == NWHILE) {
			if (exitStatus != 0)
			  break;
		} else {
			if (exitStatus == 0)
			  break;
		}
		evalTree(n->nBinary.ch2, 0);
		status = exitStatus;
		if (evalSkip)
		  goto skipping;
	}
	--loopNest;
	exitStatus = status;
}

static void evalCase(Node *n, int flags) {
	Node *cp;
	Node *patp;
	ArgList argList;
	StackMark stackMark;

	setStackMark(&stackMark);
	argList.last = &argList.list;
	expandArg(n->nCase.expr, &argList, 0);
	for (cp = n->nCase.cases; cp && evalSkip == 0; cp = cp->nCaseList.next) {
		for (patp = cp->nCaseList.pattern; patp; patp = patp->nArg.next) {
			if (caseMatch(patp, argList.list->text)) {
				if (evalSkip == 0) 
				  evalTree(cp->nCaseList.body, flags);
				goto out;
			}
		}
	}
out:
	popStackMark(&stackMark);
}

/* Search for a command. This is called before we fork so that the
 * location of the command will be available in the parent as well as
 * the child. The check for "goodName" is an overly conservative
 * check that the name will not be subject to expansion.
 */
static void preHash(Node *n) {
	CmdEntry entry;

	if (n->type == NCMD && goodName(n->nCmd.args->nArg.text)) 
	  findCommand(n->nCmd.args->nArg.text, &entry, 0);
}

/* Evaluate a pipeline. All the processes in the pipeline are children
 * of the process creating the pipeline. (This differs from some versions
 * of the shell, which make the last process in a pipeline the parent
 * of all the rest.)
 */
static void evalPipe(Node *n) {
	Job *jp;
	NodeList *lp;
	int pipeLen;
	int prevFd;
	int pip[2];

	pipeLen = 0;
	for (lp = n->nPipe.cmdList; lp; lp = lp->next) {
		++pipeLen;
	}
	INTOFF;
	jp = makeJob(pipeLen);
	prevFd = -1;
	for (lp = n->nPipe.cmdList; lp; lp = lp->next) {
		preHash(lp->node);
		pip[1] = -1;
		if (lp->next) {		/* Not the last cmd */
			if (pipe(pip) < 0) {
				if (prevFd != -1) 
				  close(prevFd);
				error("Pipe call failed");
			}
		}
		if (forkShell(jp, lp->node, n->nPipe.backgnd) == 0) {	/* Child */
			INTON;
			copyToStdin(prevFd);	/* set stdin to the previous pip[0] */
			if (pip[1] >= 0) {	/* Not the last child */
				/* Close pip[0], since:
				 *  If it is the first cmd, pipe reading is not needed.
				 *  If it is the middle cmd, then prevFd > 0, stdin has been set.
				 */
				close(pip[0]);
				copyToStdout(pip[1]); 
			}
			evalTree(lp->node, EV_EXIT);	/* Child run and exit */
		}
		if (prevFd >= 0)  
		  close(prevFd);  /* Close the previous pip[0] */
		prevFd = pip[0];  /* Mark the current pip[0], then pass it to the next child. */
		close(pip[1]);	/* Shell doesn't need to write pipe */
	}
	INTON;
	if (n->nPipe.backgnd == 0) {
		INTOFF;
		exitStatus = waitForJob(jp);
		INTON;
	}
}

static void evalFor(Node *n) {
	ArgList argList;
	Node *argp;
	StrList *sp;
	StackMark stackMark;

	setStackMark(&stackMark);
	argList.last = &argList.list;
	for (argp = n->nFor.args; argp; argp = argp->nArg.next) {
		expandArg(argp, &argList, 1);
		if (evalSkip)
		  goto out;
	}
	*argList.last = NULL;

	exitStatus = 0;
	++loopNest;
	for (sp = argList.list; sp; sp = sp->next) {
		setVar(n->nFor.var, sp->text, 0);
		evalTree(n->nFor.body, 0);
		if (evalSkip) {
			if (evalSkip == SKIP_CONT && --skipCount <= 0) {
				evalSkip = 0;
				continue;
			}
			if (evalSkip == SKIP_BREAK && --skipCount <= 0) {
				evalSkip = 0;
			}
			break;
		}
	}
	--loopNest;

out:
	popStackMark(&stackMark);
}

/* Kick off a subshell to evaluate a tree.
 */
static void evalSubshell(Node *n, int flags) {
	Job *jp;
	int backgnd = (n->type == NBACKGND);

	expRedir(n->nRedir.redirect);
	jp = makeJob(1);
	if (forkShell(jp, n, backgnd) == 0) {
		if (backgnd)
		  flags &= ~EV_TESTED;
		redirect(n->nRedir.redirect, 0);
		evalTree(n->nRedir.n, flags | EV_EXIT);	/* Never returns */
	}
	if (! backgnd) {
		INTOFF;
		exitStatus = waitForJob(jp);
		INTON;
	}
}

/* Evaluate a parse tree. The value is left in the global variable
 * existStatus.
 */
void evalTree(Node *n, int flags) {
	if (n == NULL) 
	  return;

	switch (n->type) {
		case NSEMI:
			evalTree(n->nBinary.ch1, 0);
			if (evalSkip)
			  goto out;
			evalTree(n->nBinary.ch2, flags);
			break;
		case NAND:
			evalTree(n->nBinary.ch1, EV_TESTED);
			if (evalSkip || exitStatus != 0)
			  goto out;
			evalTree(n->nBinary.ch2, flags);
			break;
		case NOR:
			evalTree(n->nBinary.ch1, EV_TESTED);
			if (evalSkip || exitStatus == 0)
			  goto out;
			evalTree(n->nBinary.ch2, flags);
			break;
		case NREDIR:
			expRedir(n->nRedir.redirect);
			redirect(n->nRedir.redirect, REDIR_PUSH);
			evalTree(n->nRedir.n, flags);
			popRedir();
			break;
		case NSUBSHELL:
			evalSubshell(n, flags);
			break;
		case NBACKGND:	/* e.g., (cat xxx) & */
			evalSubshell(n, flags);
			break;
		case NIF: {
			int status = 0;
			evalTree(n->nIf.test, EV_TESTED);
			if (evalSkip)
			  goto out;
			if (exitStatus == 0) {
				evalTree(n->nIf.ifPart, flags);
				status = exitStatus;
			} else if (n->nIf.elsePart) {
				evalTree(n->nIf.elsePart, flags);
				status = exitStatus;
			}
			exitStatus = status;
			break;
		}
		case NWHILE:
		case NUNTIL:
			evalLoop(n);
			break;
		case NFOR:
			evalFor(n);
			break;
		case NCASE:
			evalCase(n, flags);
			break;
		case NDEFUN:
			defineFunc(n->nArg.text, n->nArg.next);
			exitStatus = 0;
			break;
		case NPIPE:
			evalPipe(n);
			break;
		case NCMD:
			evalCommand(n, flags, (BackCmd *) NULL);
			break;
		default:
			out1Format("Node type = %d\n", n->type);
			flushOut(&output);
			break;
	}
out:
	if (pendingSigs)
	  doTrap();
	if ((flags & EV_EXIT) || (eflag == 1 && exitStatus && !(flags & EV_TESTED)))
	  exitShell(exitStatus);
}


/* Builtin commands. Builtin commands whose functions are closely
 * tied to evaluation are implemented here.
 */

/* No command given, or a bltin command with no arguments. Set the
 * specified variables.
 */
int bltinCmd(int argc, char **argv) {
	listSetVar(cmdEnviron);
	return exitStatus;
}

/* Handle break and continue commands. Break, continue, and return are
 * all handled by setting the evalSkip flag. The evaluation routines
 * above all check this flag, and if it is set they start skipping
 * commands rather than executing them. The variable skipCount is
 * the number of loops to break/continue, or the number of function
 * levels to return. (The latter is always 1.) It should probably
 * be an error to break out of more loops than exist, but it isn't
 * in the standard shell so we don't make it one here.
 */
int breakCmd(int argc, char **argv) {
	int n;

	n = 1;
	if (argc > 1)
	  n = number(argv[1]);
	if (n > loopNest)
	  n = loopNest;
	if (n > 0) {
		evalSkip = (**argv == 'c') ? SKIP_CONT : SKIP_BREAK;		
		skipCount = n;
	}
	return 0;
}


/* The return command */
int returnCmd(int argc, char **argv) {
	int ret;

	ret = exitStatus;
	if (argc > 1) 
	  ret = number(argv[1]);
	if (funcNest) {
		evalSkip = SKIP_FUNC;
		skipCount = 1;
	}
	return ret;
}

int trueCmd(int argc, char **argv) {
	return strcmp(argv[0], "false") == 0 ? 1 : 0;
}

int execCmd(int argc, char **argv) {
	if (argc > 1) {
		iflag = 0;	/* Exit on error */
		setInteractive(0);
		shellExec(argv + 1, environment(), pathVal(), 0);
	}
	return 0;
}

int evalCmd(int argc, char **argv) {
	char *p;
	char *concat;
	char **ap;

	if (argc > 1) {
		p = argv[1];
		if (argc > 2) {
			START_STACK_STR(concat);
			ap = argv + 2;
			for (;;) {
				while (*p) {
					ST_PUTC(*p++, concat);
				}
				if ((p = *ap++) == NULL)
				  break;
				ST_PUTC(' ', concat);
			}
			ST_PUTC('\0', concat);
			p = grabStackStr(concat);
		}
		evalString(p);
	}
	return exitStatus;
}

/* Execute a command inside back quotes. If it's a builtin command, we
 * want to save its output in a block obtained from malloc. Otherwise
 * we fork off a subprocess and get the output of the command via a pipe.
 * Should be called with interrupts off.
 */
void evalBackCmd(union Node *n, BackCmd *result) {
	int pip[2];
	Job *jp;
	StackMark stackMark;

	setStackMark(&stackMark);
	result->fd = -1;
	result->buf = NULL;
	result->numLeft = 0;
	result->jp = NULL;

	if (n == NULL) {
		/* For: `` */
	} else if (n->type == NCMD) {
		evalCommand(n, EV_BACKCMD, result);
	} else {
		if (pipe(pip) < 0) 
		  error("Pipe call failed");
		jp = makeJob(1);
		if (forkShell(jp, n, FORK_NO_JOB) == 0) {	/* Child process */
			FORCE_INTON;	
			close(pip[0]);
			copyToStdout(pip[1]);	
			evalTree(n, EV_EXIT);
		}
		/* Parent Process */
		close(pip[1]);	
		result->fd = pip[0];
		result->jp = jp;
	}
	popStackMark(&stackMark);
}




