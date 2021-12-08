#ifndef _SIGNAL_H
#define _SIGNAL_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

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

/* MINIX specific signals. These signals are not used by user processes,
 * but meant to inform system processes, like the PM, about system events.
 */
#define SIGMESS		18	/* New kernel message */
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

int sigaddset(sigset_t *set, int sig);
int sigdelset(sigset_t *set, int sig);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigismember(const sigset_t *set, int sig);
int sigpending(sigset_t *set);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigsuspend(const sigset_t *sigmask);

#endif
