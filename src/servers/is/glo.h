/* Parameters needed to keep diagnostics at IS. */
#define DIAG_BUF_SIZE	1024
extern char diagBuf[DIAG_BUF_SIZE];	/* Buffer for messages */
extern int diagNext;	/* Next index to be written */
extern int diagSize;	/* Size of all messages */

/* The parameters of the call are kept here. */
extern Message inMsg;	/* The input message itself */
extern Message outMsg;	/* The output message used for reply */
extern int who;			/* Caller's proc number */
extern int callNum;		/* System call number */
extern int dontReply;	/* Normally 0; set to 1 to inhibit reply */
