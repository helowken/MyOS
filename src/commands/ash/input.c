


#include "sys/types.h"
#include "stdio.h"	/* Defines BUFSIZ */
#include "unistd.h"
#include "shell.h"
#include "fcntl.h"
#include "errno.h"
#include "syntax.h"
#include "input.h"
#include "memalloc.h"
#include "error.h"
#include "redir.h"



/* The ParseFile structure pointed to by the global variable parseFile
 * contains information about the current file being read.
 */
MKINIT
typedef struct ParseFile {
	int lineNum;		/* Current line */
	int fd;				/* File descriptor (or -1 if string) */
	int numLeft;		/* Number of chars left in buffer */
	char *nextChar;		/* Next char in buffer */
	struct ParseFile *prev;		/* Preceding file on stack */
	char *buf;			/* Input buffer */
} ParseFile;


static int parseLineNum;		/* Input line number */
MKINIT int parseNumLeft;		/* Copy of parseFile->numLeft */
static char *parseNextChar;		/* Copy of parseFile->nextChar */
MKINIT ParseFile baseParseFile;	/* Top level input file */
char baseBuf[BUFSIZ];
ParseFile *parseFile = &baseParseFile;	/* Current input file */


#ifdef mkinit
INCLUDE "input.h"
INCLUDE "error.h"

INIT {
	extern char baseBuf[];

	baseParseFile.nextChar = baseParseFile.buf = baseBuf;
}

RESET {
	if (exception != EX_SHELL_PROC) 
	  parseNumLeft = 0;		/* Clear input buffer */
	popAllFiles();
}

SHELLPROC {
	popAllFiles();
}

#endif


/* Return to top level. 
 */
void popAllFiles() {
	//TODO
}

/* Set the input to take input from a file. If 'push' is set, push the
 * old input onto the stack first.
 */
void setInputFile(char *fileName, int push) {
	int fd, fd2;

	INTOFF;
	if ((fd = open(fileName, O_RDONLY)) < 0)
	  error("Can't open %s", fileName);
	if (fd < 10) {
		fd2 = copyFd(fd, 10);
		close(fd);
		if (fd2 < 0)
		  error("Out of file descriptors");
		fd = fd2;
	}
	setInputFd(fd, push);
	INTON;
}

/* To handle the "." command, a stack of input files is used. Pushfile
 * adds a new entry to the stack and popFile restores the previous level.
 */
static void pushFile() {
	ParseFile *pf;

	parseFile->numLeft = parseNumLeft;
	parseFile->nextChar = parseNextChar;
	parseFile->lineNum = parseLineNum;
	pf = (ParseFile *) ckMalloc(sizeof(ParseFile));
	pf->prev = parseFile;
	pf->fd = -1;
	parseFile = pf;
}

/* Like setInputFile(), but takes an open file descriptor. Call this with
 * interrupts off.
 */
void setInputFd(int fd, int push) {
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
	if (push) {
		pushFile();
		parseFile->buf = ckMalloc(BUFSIZ);
	}
	if (parseFile->fd > 0)
	  close(parseFile->fd);
	parseFile->fd = fd;
	if (parseFile->buf == NULL)
	  parseFile->buf = ckMalloc(BUFSIZ);
	parseNumLeft = 0;
	parseLineNum = 1;
}

void popFile() {
	ParseFile *pf = parseFile;

	INTOFF;
	if (pf->fd >= 0)
	  close(pf->fd);
	if (pf->buf)
	  ckFree(pf->buf);
	parseFile = pf->prev;
	ckFree(pf);
	parseNumLeft = parseFile->numLeft;
	parseNextChar = parseFile->nextChar;
	parseLineNum = parseFile->lineNum;
	INTON;
}




