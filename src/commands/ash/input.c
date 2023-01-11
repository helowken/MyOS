


#include "stdio.h"	/* Defines BUFSIZ */
#include "unistd.h"
#include "shell.h"
#include "fcntl.h"
#include "errno.h"
#include "syntax.h"
#include "input.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "redir.h"
#include "string.h"

#define EOF_NLEFT	-99		/* Value of parseNumLeft when EOF pushed back */


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


int parseLineNum;		/* Input line number */
MKINIT int parseNumLeft;		/* Copy of parseFile->numLeft */
char *parseNextChar;		/* Copy of parseFile->nextChar */
MKINIT ParseFile baseParseFile;	/* Top level input file */
char baseBuf[BUFSIZ];
ParseFile *parseFile = &baseParseFile;	/* Current input file */
char *pushedString;		/* Copy of parseNextChar when text pushed back */
int pushedNumLeft;		/* Copy of parseNumLeft when text pushed back */


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
	while (parseFile != &baseParseFile) {
		popFile();
	}
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

/* Like setInputFile, but takes input from a string
 */
void setInputString(char *string, int push) {
	INTOFF;
	if (push)
	  pushFile();
	parseNextChar = string;
	parseNumLeft = strlen(string);
	parseFile->buf = NULL;
	parseLineNum = 1;
	INTON;
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

/* Read a character from the script, returning PEOF on end of file.
 * Nul characters in the input are silently discarded.
 */
int pGetChar() {
	return pGetCharMacro();
}

/* Read a line from the script.
 */
char *pfGetStr(char *line, int len) {
	register char *p = line;
	int numLeft = len;
	int c;

	while (--numLeft > 0) {
		c = pGetCharMacro();
		if (c == PEOF) {
			if (p == line)
			  return NULL;
			break;
		}
		*p++ = c;
		if (c == '\n')
		  break;
	}
	*p = '\0';
	return line;
}

/* Undo the last call to pGetChar. Only one character may be pushed back.
 * PEOF may be pushed back.
 */
void pUngetChar() {
	++parseNumLeft;
	--parseNextChar;
}

/* Push a string back onto the input. This code doesn't work if the user
 * tries to push back more than one string at once.
 */
void pPushback(char *string, int len) {
	pushedString = parseNextChar;
	pushedNumLeft = parseNumLeft;
	parseNextChar = string;
	parseNumLeft = len;
}

/* Refill the input buffer and return the next input character;
 *
 * 1) If a string was pushed back on the input, switch back to the regular
 *    buffer.
 * 2) If an EOF was pushed back (parseNumLeft == EOF_NLEFT) or we are reading
 *    from a string so we can't refill the buffer, return EOF.
 * 3) Call read to read in the characters.
 * 4) Delete all nul characters from the buffer.
 */
int pReadBuffer() {
	register char *p, *q;
	register int i;

	if (pushedString) {
		parseNextChar = pushedString;
		pushedString = NULL;
		parseNumLeft = pushedNumLeft;
		if (--parseNumLeft >= 0)
		  return *parseNextChar++;
	}
	if (parseNumLeft == EOF_NLEFT || parseFile->buf == NULL)
	  return PEOF;
	flushOut(&output);
	flushOut(&errOut);
#if READLINE
	//TODO 
#endif
retry:
	p = parseNextChar = parseFile->buf;
	i = read(parseFile->fd, p, BUFSIZ);
	if (i <= 0) {
		if (i < 0) {
			if (errno == EINTR)
			  goto retry;
#ifdef EWOULDBLOCK
			if (parseFile->fd == 0 && errno == EWOULDBLOCK) {
				int flags = fcntl(0, F_GETFL, 0);
				if (flags >= 0 && flags & O_NONBLOCK) {
					flags &= ~O_NONBLOCK;
					if (fcntl(0, F_SETFL, flags) >= 0) {
						out2Str("sh: turning off NDELAY mode\n");
						goto retry;
					}
				}
			}
#endif
		}
		parseNumLeft = EOF_NLEFT;
		return PEOF;
	}
#if READLINE
//TODO }
#endif
	parseNumLeft = i - 1;

	/* Delete nul characters */
	for (;;) {
		if (*p++ == '\0')
		  break;
		if (--i <= 0) 
		  return *parseNextChar++;		/* No nul characters */
	}
	q = p - 1;
	while (--i > 0) {
		if (*p != '\0')
		  *q++ = *p;
		++p;
	}
	if (q == parseFile->buf)
		goto retry;		/* Buffer contained nothing but nuls */
	parseNumLeft = q - parseFile->buf - 1;
	return *parseNextChar++;
}

/* Close the file(s) that the shell is reading commands from.
 * Called after a fork is done.
 */
void closeScript() {
	popAllFiles();
	if (parseFile->fd > 0) {
		close(parseFile->fd);
		parseFile->fd = 0;
	}
}









