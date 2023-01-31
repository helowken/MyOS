



/* Values of cmdType */
#define CMD_UNKNOWN	-1		/* No entry in table for command */
#define CMD_NORMAL 0		/* Command is an executable program */
#define CMD_BUILTIN	1		/* Command is a shell builtin */
#define CMD_FUNCTION 2		/* Command is a shell function */


typedef struct {
	int cmdType;
	union Param {
		int index;
		union Node *func;
	} u;
} CmdEntry;

void shellExec(char **, char **, char *, int);
char *pathAdvance(char **, char *);
void changePath(char *);
void defineFunc(char *, union Node *);
void findCommand(char *, CmdEntry *, int);
int findBuiltin(char *);
void hashCd(void);
void unsetFunc(char *);
