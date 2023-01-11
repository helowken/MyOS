

extern char *commandName;	/* Currently executing command */
extern int exitStatus;		/* Exit status of last command */
extern struct StrList *cmdEnviron;	/* Environment for builtin command */

typedef struct {	/* Result of evalBackCmd */
	int fd;			/* File descriptor to read from */
	char *buf;		/* Buffer */
	int numLeft;	/* Number of chars in buffer */
	struct Job *jp;		/* job structure for command */
} BackCmd;

union Node;
void evalString(char *);
void evalTree(union Node *, int);
void evalBackCmd(union Node *, BackCmd *);

/* isInFunction returns nonzero if we are currently evaluating a function */
#define isInFunction()	funcNest
extern int funcNest;
