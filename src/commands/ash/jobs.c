

#include "shell.h"
#include "main.h"
#include "parser.h"
#include "nodes.h"
#include "jobs.h"
#include "options.h"
#include "trap.h"
#include "signames.h"
#include "syntax.h"
#include "input.h"
#include "memalloc.h"
#include "mystring.h"
#include "redir.h"
#include "output.h"
#include "error.h"
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

static Job *jobTable;		/* Array of jobs */
static int numJobs;		/* Size of array */
MKINIT pid_t backgndPid = -1;	


/* Returns a new job structure.
 */
Job *makeJob(int numProcs) {
	int i;
	Job *jp;
#define BATCH_SIZE	4

	for (i = numJobs, jp = jobTable; ; ++jp) {
		if (--i < 0) {
			INTOFF;
			if (numJobs == 0) {
				jobTable = ckMalloc(BATCH_SIZE * sizeof(jobTable[0]));
			} else {
				jp = ckMalloc((numJobs + BATCH_SIZE) * sizeof(jobTable[0]));
				bcopy(jobTable, jp, numJobs * sizeof(jp[0]));
				for (i = 0; i < numJobs; ++i) {
					if (jobTable[i].ps == &jobTable[i].ps0)
					  jp[i].ps = &jp[i].ps0;
				}
				ckFree(jobTable);
				jobTable = jp;
			}
			jp = jobTable + numJobs;
			for (i = BATCH_SIZE; --i >= 0; ) {
				jobTable[numJobs++].used = 0;
			}
			INTON;
			break;
		}
		if (jp->used == 0)
		  break;
	}

	INTOFF;
	jp->state = 0;
	jp->used = 1;
	jp->changed = 0;
	jp->numProcs = 0;

	if (numProcs > 1)
	  jp->ps = ckMalloc(numProcs * sizeof(ProcStat));
	else
	  jp->ps = &jp->ps0;
	INTON;
	return jp;
}

/* Return a string identifying a command to be printed by the 
 * jobs command.
 */
static char *cmdNextChar;
static int cmdNumLeft;

static void cmdPuts(char *s) {
	register char *p, *q;
	register char c;
	int subType = 0;

	if (cmdNumLeft <= 0)
	  return;
	p = s;
	q = cmdNextChar;
	while ((c = *p++) != '\0') {
		if (c == CTL_ESC) {
			*q++ = *p++;
		} else if (c == CTL_VAR) {
			*q++ = '$';
			if (--cmdNumLeft > 0)
			  *q++ = '{';
			subType = *p++;
		} else if (c == '=' && subType != 0) {
			*q++ = "}-+?="[(subType & VS_TYPE) - VS_NORMAL];
			subType = 0;
		} else if (c == CTL_END_VAR) {
			*q++ = '}';
		} else if (c == CTL_BACK_Q || 
				c == CTL_BACK_Q + CTL_QUOTE) {
			++cmdNumLeft;	/* Ignore it */
		} else {
			*q++ = c;
		}
		if (--cmdNumLeft <= 0) {
			*q++ = '.';
			*q++ = '.';
			*q++ = '.';
			break;
		}
	}
	cmdNextChar = q;
}

static void cmdText(Node *n) {
	Node *np;
	NodeList *lp;
	char *p;
	int i;
	char s[2];

	if (n == NULL)
	  return;

	switch (n->type) {
		case NSEMI:
			cmdText(n->nBinary.ch1);
			cmdPuts("; ");
			cmdText(n->nBinary.ch2);
			break;
		case NAND:
			cmdText(n->nBinary.ch1);
			cmdPuts(" && ");
			cmdText(n->nBinary.ch2);
			break;
		case NOR:
			cmdText(n->nBinary.ch1);
			cmdPuts(" || ");
			cmdText(n->nBinary.ch2);
			break;
		case NPIPE:
			for (lp = n->nPipe.cmdList; lp; lp = lp->next) {
				cmdText(lp->node);
				if (lp->next)
				  cmdPuts(" | ");
			}
			break;
		case NSUBSHELL:
			cmdPuts("(");
			cmdText(n->nRedir.n);
			cmdPuts(")");
			break;
		case NREDIR:
		case NBACKGND:
			cmdText(n->nRedir.n);
			break;
		case NIF:
			cmdPuts("if ");
			cmdText(n->nIf.test);
			cmdPuts("; then ");
			cmdText(n->nIf.ifPart);
			cmdPuts("...");
			break;
		case NWHILE:
			cmdPuts("while ");
			goto until;
		case NUNTIL:
			cmdPuts("until ");
until:
			cmdText(n->nBinary.ch1);
			cmdPuts("; do ");
			cmdText(n->nBinary.ch2);
			cmdPuts("; done");
			break;
		case NFOR:
			cmdPuts("for ");
			cmdPuts(n->nFor.var);
			cmdPuts(" in ...");
			break;
		case NCASE:
			cmdPuts("case ");
			cmdPuts(n->nCase.expr->nArg.text);
			cmdPuts(" in ...");
			break;
		case NDEFUN:
			cmdPuts(n->nArg.text);
			cmdPuts("() ...");
			break;
		case NCMD:
			for (np = n->nCmd.args; np; np = np->nArg.next) {
				cmdText(np);
				if (np->nArg.next)
				  cmdPuts(" ");
			}
			for (np = n->nCmd.redirect; np; np = np->nFile.next) {
				cmdPuts(" ");
				cmdText(np);
			}
			break;
		case NARG:
			cmdPuts(n->nArg.text);
			break;
		case NTO:
			p = ">";
			i = 1;
			goto redir;
		case NAPPEND:
			p = ">>";
			i = 1;
			goto redir;
		case NTOFD:
			p = ">&";
			i = 1;
			goto redir;
		case NFROM:
			p = "<"; 
			i = 0;
			goto redir;
		case NFROMFD:
			p = "<&";
			i = 0;
			goto redir;
redir:
			if (n->nFile.fd != i) {
				s[0] = n->nFile.fd + '0';
				s[1] = '\0';
				cmdPuts(s);
			}
			cmdPuts(p);
			if (n->type == NTOFD || n->type == NFROMFD) {
				s[0] = n->nDup.dupFd + '0';
				s[1] = '\0';
				cmdPuts(s);
			} else {
				cmdText(n->nFile.fileName);
			}
			break;
		case NHERE:
		case NXHERE:
			cmdPuts("<<...");
			break;
		default:
			cmdPuts("???");
			break;
	}
}

static char *commandText(Node *n) {
	char *name;

	cmdNextChar = name = ckMalloc(50);
	cmdNumLeft = 50 - 4;
	cmdText(n);
	*cmdNextChar = '\0';
	return name;
}

/* Fork of a subshell. If we are doing job control, give the subshell its
 * own process group. Jp is a job structure that the job is to be added to.
 * N is the command that will be evaluated by the child. Both jp and n may
 * be NULL. The mode parameter can be one of the following:
 *	FORK_FG - Fork off a foreground process.
 *	FORK_BG - Fork off a background process.
 *	FORK_NO_JOB - Like FORK_FG, but don't give the process its own
 *	              process group even if job control is on.
 *
 * When job control is turned off, background processes have their standard
 * input redirected to /dev/null (except for the second and later processes
 * in a pipeline).
 */
int forkShell(Job *jp, Node *n, int mode) {
	int pid;

	INTOFF;
	pid = fork();
	if (pid == -1) {
		INTON;
		error("Cannot fork");
	} 
	if (pid == 0) {
		Job *p;
		int wasRoot;
		int i;

		wasRoot = rootShell;
		rootShell = 0;
		for (i = numJobs, p = jobTable; --i >= 0; ++p) {
			if (p->used) 
			  freeJob(p);
		}
		closeScript();
		INTON;
		clearTraps();

		if (mode == FORK_BG) {
			ignoreSig(SIGINT);
			ignoreSig(SIGQUIT);
			if ((jp == NULL || jp->numProcs == 0) &&
					! isFd0Redirected()) {
				close(STDIN_FILENO);
				if (open("/dev/null", O_RDONLY) != 0)
				  error("Can't open /dev/null");
			}
		}
		if (wasRoot && iflag) {
			setSignal(SIGINT);
			setSignal(SIGQUIT);
			setSignal(SIGTERM);
		}
		return pid;
	}

	if (mode == FORK_BG)
	  backgndPid = pid;		/* Set $! */
	if (jp) {
		ProcStat *ps = &jp->ps[jp->numProcs++];
		ps->pid = pid;
		ps->status = -1;
		ps->cmd = nullStr;
		if (iflag && rootShell && n) 
		  ps->cmd = commandText(n);
	}
	INTON;
	return pid;
}

static int waitProc(int block, int *status) {
	return waitpid(-1, status, block == 0 ? WNOHANG : 0);
}

/* Wait for a process to terminate. */
static int doWait(int block, Job *job) {
	int pid;
	int status;
	ProcStat *sp;
	Job *jp, *thisJob;
	int done;
	int stopped;
	int sig;

	do {
		pid = waitProc(block, &status);
	} while (pid == -1 && errno == EINTR);

	if (pid <= 0)
	  return pid;

	INTOFF;
	thisJob = NULL;
	for (jp = jobTable; jp < jobTable + numJobs; ++jp) {
		if (jp->used) {
			done = 1;
			stopped = 1;
			for (sp = jp->ps; sp < jp->ps + jp->numProcs; ++sp) {
				if (sp->pid == -1)
				  continue;
				if (sp->pid == pid) {
					sp->status = status;
					thisJob = jp;
				}
				if (sp->status == -1)
				  stopped = 0;
				else if (WIFSTOPPED(sp->status))
				  done = 0;
			}
			if (stopped) {	/* Stopped or done */
				int state = done ? JOB_DONE : JOB_STOPPED;
				if (jp->state != state) 
				  jp->state = state;
			}
		}
	}
	INTON;

	if (! rootShell || ! iflag || (job && thisJob == job)) {
		if (WIFSIGNALED(status)) {
			sig = WTERMSIG(status);
			if (sig != 0 && sig != SIGINT && sig != SIGPIPE) {
				if (thisJob != job) 
				  outFormat(out2, "%d: ", pid);
				if (sig <= MAX_SIG && sigMsg[sig])
				  out2Str(sigMsg[sig]);
				else
				  outFormat(out2, "Signal %d", sig);

				if (WCOREDUMP(status))
				  out2Str(" - core dumped");
				flushOut(&errOut);
			}
		}
	} else {
		if (thisJob)
		  thisJob->changed = 1;
	}
	return pid;
}

/* Wait for job to finish.
 *
 * Under job control we have the problem that while a child process is
 * running interrupts generated by the user are sent to the child but not
 * to the shell. This means that an infinite loop started by an inter-
 * active user may be hard to kill. With job control turned off, an 
 * interactive program catches interrupts, the user doesn't want
 * these interrupts to also abort the loop. The approach we take here
 * is to have the shell ignore interrupt signals while waiting for a
 * foreground process to terminate, and then send itself an interrupt
 * signal if the child process was terminated by an interrupt signal.
 * Unfortunately, some programs want to do a bit of cleanup and then
 * exit on interrupt; unless these processes terminate themselves by
 * sending a signal to themselves (instead of calling exit) they will
 * confuse this approach.
 */
int waitForJob(Job *jp) {
	int status;
	int st;

	INTOFF;
	while (jp->state == 0 && doWait(1, jp) != -1) {
	}
	status = jp->ps[jp->numProcs - 1].status;
	/* Convert to 8 bits */
	if (WIFEXITED(status))
	  st = WEXITSTATUS(status);
	else
	  st = (status & 0x7F) + 128;

	if (jp->state == JOB_DONE)
	  freeJob(jp);

	CLEAR_PENDING_INT;
	if (WTERMSIG(status) == SIGINT)
	  kill(getpid(), SIGINT);
	INTON;

	return st;
}

/* Make a job structure as unused. */
void freeJob(Job *jp) {
	ProcStat *ps;
	int i;

	INTOFF;
	for (i = jp->numProcs, ps = jp->ps; --i >= 0; ++ps) {
		if (ps->cmd != nullStr)
		  ckFree(ps->cmd);
	}
	if (jp->ps != &jp->ps0)
	  ckFree(jp->ps);
	jp->used = 0;
	INTON;
}

void showJobs(int change) {
	int jobNum;
	int procNum;
	int i, sig;
	Job *jp;
	ProcStat *ps;
	int col;
	char s[64];

	while (doWait(0, (Job *) NULL) > 0) {
	}
	for (jobNum = 1, jp = jobTable; jobNum <= numJobs; ++jobNum, ++jp) {
		if (! jp->used)
		  continue;
		if (jp->numProcs == 0) {
			freeJob(jp);
			continue;
		}
		if (change && ! jp->changed)
		  continue;

		procNum = jp->numProcs;
		for (ps = jp->ps; ; ++ps) {	/* For each process */
			if (ps == jp->ps)
			  formatStr(s, 64, "[%d] %d ", jobNum, ps->pid);
			else
			  formatStr(s, 64, "    %d ", ps->pid);
			out1Str(s);
			col = strlen(s);

			s[0] = '\0';
			if (ps->status == -1) {
				/* Don't print anything */				
			} else if (WIFEXITED(ps->status)) {
				formatStr(s, 64, "Exit %d", WEXITSTATUS(ps->status));
			} else {
				i = ps->status;
				sig = WTERMSIG(i); 
				if (sig <= MAX_SIG && sigMsg[sig])
				  scopy(sigMsg[sig], s);
				else
				  formatStr(s, 64, "Signal %d", sig);
				if (WCOREDUMP(i))
				  strcat(s, " (core dumped)");
			}
			out1Str(s);
			col += strlen(s);

			do {
				out1Char(' ');
				++col;
			} while (col < 30);
			out1Str(ps->cmd);
			out1Char('\n');

			if (--procNum <= 0)
			  break;
		}
		jp->changed = 0;
		if (jp->state == JOB_DONE)
		  freeJob(jp);
	}
}

/* Convert a job name to a job structure.
 */
static Job *getJob(char *name) {
	int jobNum;
	register Job *jp;
	int pid;
	int i;

	if (name == NULL) {
		error("No current job");
	} else if (name[0] == '%') {
		if (isDigit(name[1])) {
			jobNum = number(name + 1);
			if (jobNum > 0 && jobNum <= numJobs && 
					jobTable[jobNum - 1].used)
			  return &jobTable[jobNum - 1];
		} else {
			register Job *found = NULL;
			for (jp = jobTable, i = numJobs; --i >= 0; ++jp) {
				if (jp->used && jp->numProcs > 0 && 
						prefix(name + 1, jp->ps[0].cmd)) {
					if (found)
					  error("%s: ambiguous", name);
					found = jp;
				}
			}
			if (found)
			  return found;
		}
	} else if (isNumber(name)) {
		pid = number(name);
		for (jp = jobTable, i = numJobs; --i >= 0; ++jp) {
			if (jp->used && jp->numProcs > 0 &&
					jp->ps[jp->numProcs - 1].pid == pid)
			  return jp;
		}
	}
	error("No such job: %s", name);
	return NULL;
}

int waitCmd(int argc, char **argv) {
	Job *job;
	int status;
	Job *jp;

	if (argc > 1)
	  job = getJob(argv[1]);
	else
	  job = NULL;

	for (;;) {	/* Loop until process terminated or stopped */
		if (job != NULL) {
			if (job->state) {
				status = job->ps[job->numProcs - 1].status;
				if (WIFEXITED(status))
				  status = WEXITSTATUS(status);
				else
				  status = WTERMSIG(status) + 128;
				if (! iflag)
				  freeJob(job);
				return status;
			}
		} else {
			for (jp = jobTable; ; ++jp) {
				if (jp >= jobTable + numJobs) 	/* No running procs */
				  return 0;
				if (jp->used && jp->state == 0)
				  break;
			}
		}
		doWait(1, NULL);
	}
}

int jobIdCmd(int argc, char **argv) {
	Job *jp;
	int i;

	jp = getJob(argv[1]);
	for (i = 0; i < jp->numProcs; ) {
		out1Format("%d", jp->ps[i].pid);
		out1Char(++i < jp->numProcs ? ' ' : '\n');
	}
	return 0;
}

int jobsCmd(int argc, char **argv) {
	showJobs(0);
	return 0;
}
