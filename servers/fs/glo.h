#ifdef _TABLE
#undef EXTERN
#define EXTERN
#endif

/* File System global variables */
EXTERN struct FProc *fp;	/* Pointer to caller's FProc */
EXTERN int superUser;		/* 1 if caller is superUser, else 0 */
EXTERN dev_t rootDev;		/* Device number of the root device */

/* The parameters of the call are kept here. */
EXTERN Message inMsg;		/* The input message itself */
EXTERN Message outMsg;		/* The output message used for reply */
EXTERN int who;				/* Caller's proc number */
EXTERN int callNum;			/* System call number */



