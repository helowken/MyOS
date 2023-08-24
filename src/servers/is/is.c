/* System Information Service.
 * This service handles the various debugging dumps, such as the process
 * table, so that these no longer directly touch kernel memory. Instead, the
 * system task is asked to copy some table in local memory.
 */

#include "is.h"

/* Allocate space for the global variables. */
Message inMsg;		
Message outMsg;
int who;
int callNum;

extern int errno;


static void initServer(int argc, char **argv) {
/* Initialize the information service. */
	int fKeys, sfKeys;
	int i, s;

	/* Set key mappings. IS takes all of F1-F12 and Shift+F1-F6. */
	fKeys = sfKeys = 0;
	for (i = 1; i <= 12; ++i) {
		bitSet(fKeys, i);
	}
	for (i = 1; i<= 6; ++i) {
		bitSet(sfKeys, i);
	}
	if ((s = fKeyMap(&fKeys, &sfKeys)) != OK)
	  report("IS", "warning, fKeyMap failed:", s);
}

static void getWork() {
	int status;

	if ((status = receive(ANY, &inMsg)) != OK)
	  panic("IS", "failed to receive message!", status);

	who = inMsg.m_source;	
	callNum = inMsg.m_type;
}

static void exitServer() {
/* Shut down the information service. */
	int fKeys, sfKeys;
	int i, s;

	/* Release the function key mappings requested in initServer().
	 * IS took all of F1-F12 and Shift+F1-F6.
	 */
	fKeys = sfKeys = 0;
	for (i = 1; i <= 12; ++i) {
		bitSet(fKeys, i);
	}
	for (i = 1; i<= 6; ++i) {
		bitSet(sfKeys, i);
	}
	if ((s = fKeyUnmap(&fKeys, &sfKeys)) != OK)
	  report("IS", "warning, fKeyUnmap failed:", s);

	/* Done. Now exit. */
	exit(EXIT_SUCCESS);
}

static void reply(int who, int result) {
	int s; 

	outMsg.m_type = result;		
	if ((s = send(who, &outMsg)) != OK)
	  panic("IS", "unable to send reply", s);
}

int main(int argc, char **argv) {
/* This is the main routine of this service. The main loop consists of
 * three major activities: getting new work, processing the work, and
 * sending the reply. The loop never terminates, unless a panic occurs.
 */
	int result;
	sigset_t sigset;

	/* Initialize the server, then go to work. */
	initServer(argc, argv);

	/* Main loop - get work and do it, forever. */
	while (TRUE) {
		/* Wait for incoming message, sets 'callNum' and 'who'. */
		getWork();

		switch (callNum) {
			case SYS_SIG:
				sigset = (sigset_t) inMsg.NOTIFY_ARG;
				if (sigismember(&sigset, SIGTERM) || 
						sigismember(&sigset, SIGKSTOP)) {
					exitServer();
				}
				continue;
			case FKEY_PRESSED:
				result = doFKeyPressed(&inMsg);
				break;
			default:
				report("IS", "warning, got illegal request from %d\n", who);
				result = EINVAL;
		}

		/* Finally send reply message, unless disabled. */
		if (result != EDONTREPLY)
		  reply(who, result);
	}
	return OK;		/* Shouldn't come here */
}
