


typedef struct {
	int numParam;	/* Number of positional parameters (without $0) */
	char malloc;	/* True if parameter list dynamicly allocated */
	char **params;	/* Parameter list */
	char **optNext;	/* Next parameter to be processed by getopts */
	char *optPtr;	/* Used by getopts */
} ShellParam;


#define eFlag	optVal[0]
#define fFlag	optVal[1]
#define IFlag	optVal[2]
#define iFlag	optVal[3]
#define jFlag	optVal[4]
#define nFlag	optVal[5]
#define sFlag	optVal[6]
#define xFlag	optVal[7]
#define zFlag	optVal[8]
#define vFlag	optVal[9]



#define NUM_OPTS	10

#ifdef DEFINE_OPTIONS
const char optChar[NUM_OPTS + 1] = "efIijnsxzv";	/* Shell flags */
char optVal[NUM_OPTS + 1];
#else
extern const char optChar[NUM_OPTS + 1];
extern char optVal[NUM_OPTS + 1];
#endif

extern int isatty(int fd);

void procArgs(int, char **);
void setParam(char **);
void freeParam(ShellParam *);
