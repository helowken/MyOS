

extern char *commandName;	/* Currently executing command */
extern int exitStatus;		/* Exit status of last command */

union Node;

void evalString(char *);
void evalTree(union Node *, int);

/* isInFunction returns nonzero if we are currently evaluating a function */
#define isInFunction()	funcNest
extern int funcNest;
