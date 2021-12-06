#ifndef _SIGNAL_H
#define _SIGNAL_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

#define NSIG		20	/* Number of signals used */

int sigaddset(sigset_t *set, int sig);
int sigdelset(sigset_t *set, int sig);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigismember(const sigset_t *set, int sig);
int sigpending(sigset_t *set);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigsuspend(const sigset_t *sigmask);

#endif
