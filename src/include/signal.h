#ifndef _SIGNAL_H
#define _SIGNAL_H

#ifdef _POSIX_SOURCE
#ifndef _TYPES_H
#include "sys/types.h"
#endif

#ifndef _SIGSET_T
#define _SIGSET_T
typedef unsigned long	sigset_t;
#endif
#endif	/* _POSIX_SOURCE */

typedef int sig_atomic_t;

#define NSIG		20	/* Number of signals used */

#define	SIGHUP		1	/* Hangup */
#define	SIGINT		2	/* Interrupt (DEL) */
#define SIGQUIT		3	/* Quit */
#define SIGILL		4	/* Illegal instruction */
#define SIGTRAP		5	/* Trace trap (not reset when caught) */
#define SIGABRT		6	/* IOT instruction */
#define SIGIOT		6	/* SIGABRT for people who speak PDP-11 */
#define SIGUNUSED	7	/* Spare code */
#define SIGFPE		8	/* Floating point exception */
#define SIGKILL		9	/* Kill (cannot be caught or ignored) */
#define SIGUSR1		10	/* User defined signal # 1 */
#define SIGSEGV		11	/* Segmentation violation */
#define	SIGUSR2		12	/* User defined signal # 2 */
#define SIGPIPE		13	/* Write on a pipe with no one to read it */
#define SIGALRM		14	/* Alarm clock */
#define SIGTERM		15	/* Software termination signal from kill */
#define SIGCHLD		17	/* Child process terminated or stopped */

#define SIGEMT		7	/* Obsolete */
#define SIGBUS		10	/* Obsolete */

/* MINIX specific signals. These signals are not used by user processes,
 * but meant to inform system processes, like the PM, about system events.
 */
#define SIGKMESS	18	/* New kernel message */
#define	SIGKSIG		19	/* Kernel signal pending */
#define SIGKSTOP	20	/* Kernel shutting down */

/* POSIX requires the following signals to be defiend, even if they are
 * not supported. Here are the definitions, but they are not supported.
 */
#define SIGCONT		18	/* Continue if stopped */
#define SIGSTOP		19	/* Stop signal */
#define	SIGTSTP		20	/* Interactive stop signal */
#define SIGTTIN		21	/* Background process wants to read */
#define SIGTTOU		22	/* Background process wants to write */

typedef void (*SigHandler)(int);

/* Macros used as function pointers. */
#define SIG_ERR		((SigHandler) -1)	/* Error return */
#define SIG_DFL		((SigHandler)  0)	/* Default signal handling */
#define SIG_IGN		((SigHandler)  1)	/* Ignore signal */
#define SIG_HOLD	((SigHandler)  2)	/* Block signal */
#define SIG_CATCH	((SigHandler)  3)	/* Catch signal */
#define SIG_MESS	((SigHandler)  4)	/* Pass as message (MINIX) */

#ifdef _POSIX_SOURCE
struct sigaction {			
	SigHandler sa_handler;	/* SIG_DFL, SIG_IGN, or pointer to function */
	sigset_t sa_mask;		/* Signals to be blocked during handler */
	int sa_flags;			/* Special flags */
};

/* Fields for sa_flags. */
#define SA_ONSTACK		0x0001	/* Deliver signal on alternate stack */
#define SA_RESETHAND	0x0002	/* Reset signal handler when signal caught */
#define SA_NODEFER		0x0004	/* Don't block signal while catching it */
#define SA_RESTART		0x0008	/* Automatic system call restart */
#define SA_SIGINFO		0x0010	/* Extended signal handling */
#define SA_NOCLDWAIT	0x0020	/* Don't create zombies */
#define SA_NOCLDSTOP	0x0040	/* Don't receive SIGCHLD when child stops */

/* POSIX requires these values for use with sigprocmask(2). */
#define SIG_BLOCK		0	/* For blocking signals */
#define SIG_UNBLOCK		1	/* For unblocking signals */
#define SIG_SETMASK		2	/* For setting the signal mask */
#define SIG_INQUIRE		4	/* For internel use only */
#endif	/* _POSIX_SOURCE */

int raise(int sig);
SigHandler signal(int sig, SigHandler func);

#ifdef _POSIX_SOURCE
int kill(pid_t pid, int sig);
int sigaction(int sig, const struct sigaction *sa, struct sigaction *oldSa);

int sigaddset(sigset_t *set, int sig);
int sigdelset(sigset_t *set, int sig);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigismember(const sigset_t *set, int sig);
int sigpending(sigset_t *set);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigsuspend(const sigset_t *sigmask);
#endif	/* _POSIX_SOURCE */

#endif
