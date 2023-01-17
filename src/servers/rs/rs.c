/* Reincarnation Server. This server starts new system services and detects
 * they are exiting. In case of errors, system services can be restarted.
 */

#include "rs.h"

/* Allocate space for the global variables. */
static Message inMsg;		/* The input message itself. */
static Message outMsg;		/* The output message used for reply */
static int who;				/* Caller's proc number */
static int callNum;			/* System call number */

static void initServer() {
	struct sigaction sa;
	int sigs[] = {SIGCHLD, SIGTERM, SIGABRT, SIGHUP};
	int i;

	/* Install signal handlers. Ask PM to transform signal into message. */
	sa.sa_handler = SIG_MESS;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	for (i = 0; i < 4; ++i) {
		if (sigaction(sigs[i], &sa, NULL) < 0)
		  panic("RS", "sigaction failed", errno);
	}
}

static void getWork() {
	int status = 0;

	status = receive(ANY, &inMsg);	/* This blocks until message arrives. */
	if (status != OK) 
	  panic("RS", "failed to receive message!", status);
	who = inMsg.m_source;		/* Message arrived! Set sender */
	callNum = inMsg.m_type;		/* Set function call number */
}

static void reply(int who, int result) {
	int status;

	outMsg.m_type = result;			/* Build reply message */
	status = send(who, &outMsg);	/* Send the message */
	if (status != OK)
	  panic("RS", "unable to send reply!", status);
}

int main() {
/* This is the main routine of this service. The main loop consists of
 * three major activities: getting new work, processing the work, and
 * sending the reply. The loop never terminates, unless a panic occurs.
 */
	int result;
	sigset_t sigset;

	/* Initialize the server, then go to work. */
	initServer();

	/* Main loop - get work and do it, forever. */
	while (TRUE) {
		/* Wait for incoming message, sets 'callNum' and 'who'. */
		getWork();

		switch (callNum) {
			case SYS_SIG:
				/* Signals are passed by means of a notification message 
				 * from SYSTEM. Extract the map of pending signals from 
				 * the notification argument.
				 */
				sigset = (sigset_t) inMsg.NOTIFY_ARG;

				if (sigismember(&sigset, SIGCHLD)) {
					/* A child of this server exited. Take action. */
					doExit(&inMsg);
				}
				if (sigismember(&sigset, SIGUSR1)) 
				  doStart(&inMsg);
				if (sigismember(&sigset, SIGTERM)) {
					/* Nothing to do on shutdown. */
				}
				if (sigismember(&sigset, SIGKSTOP)) {
					/* Nothing to do on shutdown. */
				}
				continue;
			case SRV_UP:
				result = doStart(&inMsg);
				break;
			case SRV_DOWN:
				result = doStop(&inMsg);
				break;
			default:
				printf("Warning, RS got unexpected request %d from %d\n",
							inMsg.m_type, inMsg.m_source);
				result = EINVAL;
		}

		/* Finally send reply message, unless disabled. */
		if (result != EDONTREPLY)
		  reply(who, result);
	}
}
