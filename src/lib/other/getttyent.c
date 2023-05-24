#define nil	0
#include <string.h>
#include <sys/types.h>
#include <ttyent.h>
#include <unistd.h>
#include <fcntl.h>

#define arraySize(a)	(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)	((a) + arraySize(a))

static char TTY_TAB[] = "/etc/ttytab";	/* The table of terminal devices. */

static char buf[512];			
static char ttLine[256];		/* One line from the ttytab file. */
static char *ttArgv[32];		/* Compound arguments. */
static struct ttyent entry;		/* Entry to fill and return. */
static int ttFd = -1;			/* FD to the file. */
static char *bufPtr;			
static ssize_t bufLen = 0;		/* Remaining characters in buf. */
static char *linePtr;			
static char **argvPtr;

void endttyent(void) {
/* Close the ttytab file. */
	if (ttFd >= 0) {
		close(ttFd);
		ttFd = -1;
		bufLen = 0;
	}
}

int setttyent(void) {
/* Open the ttytab file. */
	if (ttFd >= 0)
	  endttyent();

	if ((ttFd = open(TTY_TAB, O_RDONLY)) < 0)
	  return -1;

	fcntl(ttFd, F_SETFD, fcntl(ttFd, F_GETFD) | FD_CLOEXEC);
	return 0;
}

static int getLine() {
/* Get one line from the ttytab file, return 0 if bad or EOF. */
	linePtr = ttLine;
	argvPtr = ttArgv;

	do {
		if (bufLen == 0) {
			if ((bufLen = read(ttFd, buf, sizeof(buf))) <= 0)
			  return 0;
			bufPtr = buf;
		}

		if (linePtr == arrayLimit(ttLine))
		  return 0;

		--bufLen;
	} while ((*linePtr++ = *bufPtr++) != '\n');

	linePtr = ttLine;
	return 1;
}

static int white(int c) {
/* Whitespace? */
	return c == ' ' || c == '\t';
}

static char *scanWhite(int quoted) {
/* Scan for a field separator in a line, return the start of the field.
 * "quoted" is set if we have to watch out for double quotes.
 */
	char *field, *last;

	while (white(*linePtr)) {
		++linePtr;
	}
	if (! quoted && *linePtr == '#')
	  return nil;

	field = linePtr;
	for (;;) {
		last = linePtr;
		if (*linePtr == 0)
		  return nil;
		if (*linePtr == '\n')
		  break;
		if (quoted && *linePtr == '"')
		  return field;
		if (white(*linePtr++))
		  break;
	}
	*last = 0;
	return *field == 0 ? nil : field;
}

static char **scanQuoted() {
/* Read a field that may be a quoted list of words. */
	char *p, **field = argvPtr;

	while (white(*linePtr)) {
		++linePtr;
	}

	if (*linePtr == '"') {
		/* Quoted list of words. */
		++linePtr;
		while ((p = scanWhite(1)) != nil && *p != '"') {
			if (argvPtr == arrayLimit(ttArgv))
			  return nil;
			*argvPtr++ = p;
		}
		if (*linePtr == '"')
		  *linePtr++ = 0;
	} else {
		/* Just one word. */
		if ((p = scanWhite(0)) == nil)
		  return nil;
		if (argvPtr == arrayLimit(ttArgv))
		  return nil;
		*argvPtr++ = p;
	}
	if (argvPtr == arrayLimit(ttArgv))
	  return nil;
	*argvPtr++ = nil;
	return field;
}

struct ttyent *getttyent(void) {
/* Read one entry from the ttytab file. */
	
	/* Open the file if not yet open. */
	if (ttFd < 0 && setttyent() < 0)
	  return nil;

	/* Look for a line with something on it. */
	for (;;) {
		if (! getLine())	/* EOF or corrupt. */
		  return nil;
		if ((entry.ty_name = scanWhite(0)) == nil)
		  continue;
		entry.ty_type = scanWhite(0);
		entry.ty_getty = scanQuoted();
		entry.ty_init = scanQuoted();
		return &entry;
	}
}

struct ttyent *getttynam(const char *name) {
/* Return the ttytab file entry for a given tty. */
	struct ttyent *tty;

	endttyent();
	while ((tty = getttyent()) != nil &&
			strcmp(tty->ty_name, name) != 0) {
	}
	endttyent();
	return tty;
}


