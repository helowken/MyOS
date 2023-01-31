


typedef struct {
	int numParam;	/* Number of positional parameters (without $0) */
	char malloc;	/* True if parameter list dynamicly allocated */
	char **params;	/* Parameter list */
	char **optNext;	/* Next parameter to be processed by getopts */
	char *optPtr;	/* Used by getopts */
} ShellParam;


#define eflag	optVal[0]	/* Cause the shell to exit when a command terminates with a nonzero exit status. */
#define fflag	optVal[1]	/* Turn off file name generation. */
#define Iflag	optVal[2]	/* Cause the shell to ignore end of file conditions. */
#define iflag	optVal[3]	/* Make the shell interactive. */
#define jflag	optVal[4]	/* Turns on Berkeley job control, on systems that support it. */
#define nflag	optVal[5]	/* Cause the shell to read commands but not execute them. (For checking syntax) */
#define sflag	optVal[6]	/* If set, when the shell starts up, the shell reads commands from its stdin. */
#define xflag	optVal[7]	/* If set, the shell will print out each command before executing it. */
#define zflag	optVal[8]	/* If set, the file name generation process may generate zero files. */
#define vflag	optVal[9]



#define NUM_OPTS	10

#ifdef DEFINE_OPTIONS
const char optChar[NUM_OPTS + 1] = "efIijnsxzv";	/* Shell flags */
char optVal[NUM_OPTS + 1];
#else
extern const char optChar[NUM_OPTS + 1];
extern char optVal[NUM_OPTS + 1];
#endif

extern char *minusC;	/* Argument to -c option: the shell executes the specified shell command */
extern char *arg0;		/* $0 */
extern char **argPtr;	/* Argument list for builtin commands */
extern char *optArg;	/* Set by nextOpt */
extern char *optPtr;	/* Used by nextOpt */
extern ShellParam shellParam;	/* $@ */
extern int editable;	/* isatty(0) && isatty(1) */

extern int isatty(int fd);

void procArgs(int, char **);
void setParam(char **);
void freeParam(ShellParam *);
int nextOpt(char *);

