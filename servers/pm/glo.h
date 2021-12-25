#ifdef _TABLE
#undef EXTERN
#define EXTERN
#endif

/* Global variables. */
EXTERN struct MProc *currMp;	/* Pointer to 'mprocTable' slot of current process */
EXTERN int procsInUse;			/* How many processes are marked as IN_USE */
EXTERN char monitorParams[128 * sizeof(char *)];	/* Boot minitor parameters */
EXTERN KernelInfo kernelInfo;	/* Kernel information */

/* The parameters of the call are kept here */
EXTERN Message inputMsg;		/* The incoming message itself is kept here. */
EXTERN int who;					/* Caller's proc number */
EXTERN int callNum;				/* System call number */

extern int (*callVec[])();		/* System call handlers */
extern char coreName[];			/* File name where core images are produced */
EXTERN sigset_t coreSigSet;		/* Which signals cause core images */
EXTERN sigset_t ignoreSigSet;	/* Which signals are by default ignored */


