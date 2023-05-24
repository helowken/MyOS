#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define EOS '\0'
#define BOOLEAN int
#define TRUE	1
#define FALSE	0

#define SWAB	0x0001
#define LCASE	0x0002
#define UCASE	0x0004
#define NOERROR	0x0008
#define SYNC	0x0010
#define SILENT	0x0020
#define BLANK	' '
#define DEFAULT	512

static char *pch, *errorPtr;
static unsigned blockSize, skipBlocks, seekBlocks, countBlocks;
static int seekSeen = FALSE;
static unsigned iBlockSize = DEFAULT;
static unsigned oBlockSize = DEFAULT;
static unsigned files = 1;
static char *iFileName = NULL;
static char *oFileName = NULL;

static int convFlag = 0;
static int flag = 0;
static int iFd, oFd, iBufCount, oBufCount;
static char *iBuf, *oBuf, *oPtr;
static unsigned nInFull, nInPartial, nOutFull, nOutPartial;
static unsigned nTruncated;

BOOLEAN is(char *pc) {
	register char *ps = pch;

	while (*ps++ == *pc++) {
		if (*pc == EOS) {
			pch = ps;
			return TRUE;
		}
	}
	return FALSE;
}

#define BIG_NUM	2147483647

int num() {
	long ans;
	register char *pc;

	pc = pch;
	ans = 0L;
	while ((*pc >= '0') && (*pc <= '9')) {
		ans = (long) ((*pc++ - '0') + (ans * 10));
	}
	while (TRUE) {
		switch (*pc++) {
			case 'w':
				ans *= 2L;
				continue;
			case 'b':
				ans *= 512L;
				continue;
			case 'k':
				ans *= 1024L;
				continue;
			case 'x':
				pch = pc;
				ans *= (long) num();
			case EOS:
				if (ans >= BIG_NUM || ans < 0) {
					fprintf(stderr, "dd: argument %s out of range\n", 
								errorPtr);
					exit(1);
				}
				return (int) ans;
		}
	}
}

static void putOut() {
	int n;

	if (oBufCount == 0)
	  return;
	if (oBufCount == oBlockSize)
	  ++nOutFull;
	else
	  ++nOutPartial;
	if ((n = write(oFd, oBuf, oBufCount)) != oBufCount) {
		if (n == -1) 
		  fprintf(stderr, "dd: Write error: %s\n", strerror(errno));
		else
		  fprintf(stderr, "dd: Short write: %d instead of %d\n", n, oBufCount);
		exit(1);
	}
	oBufCount = 0;
}

static int upLowCase(int c) {
	int ans = c;

	if ((convFlag & UCASE) && (c >= 'a') && (c <= 'z'))
	  ans += 'A' - 'a';
	if ((convFlag & LCASE) && (c >= 'A') && (c <= 'Z'))
	  ans += 'a' - 'A';
	return ans;
}

static void nullFunc(int c) {
	*oPtr++ = c;
	if (++oBufCount >= oBlockSize) {
		putOut();
		oPtr = oBuf;
	}
}

static void caseNullFunc(int c) {
	c = upLowCase(c);
	nullFunc(c);
}

static void statistics() {
	if (convFlag & SILENT)
	  return;
	fprintf(stderr, "\n");
	fprintf(stderr, "%u+%u records in\n", nInFull, nInPartial);
	fprintf(stderr, "%u+%u records out\n", nOutFull, nOutPartial);
	if (nTruncated)
	  fprintf(stderr, "%d truncated records\n", nTruncated);
}

static void over(int sig) {
	statistics();
	if (sig != 0) {
		signal(sig, SIG_DFL);
		raise(sig);
	}
	exit(0);
}

int main(int argc, char **argv) {
	void (*convertFunc)(int);
	char *iPtr;
	int i;

	convertFunc = nullFunc;
	--argc;
	++argv;
	while (argc-- > 0) {
		pch = *(argv++);	
		if (is("ibs=")) {
			errorPtr = pch;
			iBlockSize = num();
			continue;
		}
		if (is("obs=")) {
			errorPtr = pch;
			oBlockSize = num();
			continue;
		}
		if (is("bs=")) {
			errorPtr = pch;
			blockSize = num();
			continue;
		}
		if (is("if=")) {
			iFileName = pch;
			continue;
		}
		if (is("of=")) {
			oFileName = pch;
			continue;
		}
		if (is("skip=")) {
			errorPtr = pch;
			skipBlocks = num();
			continue;
		}
		if (is("seek=")) {
			errorPtr = pch;
			seekBlocks = num();
			seekSeen = TRUE;
			continue;
		}
		if (is("count=")) {
			errorPtr = pch;
			countBlocks = num();
			continue;
		}
		if (is("files=")) {
			errorPtr = pch;
			files = num();
			continue;
		}
		if (is("conv=")) {
			while (*pch != EOS) {
				if (is("lcase")) {
					convFlag |= LCASE;
					continue;
				}
				if (is("ucase")) {
					convFlag |= UCASE;
					continue;
				}
				if (is("noerror")) {
					convFlag |= NOERROR;
					continue;
				}
				if (is("sync")) {
					convFlag |= SYNC;
					continue;
				}
				if (is("swab")) {
					convFlag |= SWAB;
					continue;
				}
				if (is("silent")) {
					convFlag |= SILENT;
					continue;
				}
				if (is(","))
				  continue;
				fprintf(stderr, "dd: bad argument: %s\n", pch);
				exit(1);
			}
			if (*pch == EOS)
			  continue;
		}
		fprintf(stderr, "dd: bad argument: %s\n", pch);
		exit(1);
	}
	if (convertFunc == nullFunc && (convFlag & (UCASE | LCASE)))
	  convertFunc = caseNullFunc;

	if ((iFd = (iFileName ? open(iFileName, O_RDONLY) : dup(STDIN_FILENO))) < 0) {
		fprintf(stderr, "dd: Can't open %s: %s\n",
				iFileName ? iFileName : "stdin", strerror(errno));
		exit(1);
	}
	if ((oFd = (oFileName ? open(oFileName, seekSeen ? O_WRONLY | O_CREAT : 
							O_WRONLY | O_CREAT | O_TRUNC, 0666) : 
						dup(STDOUT_FILENO))) < 0) {
		fprintf(stderr, "dd: Can't open %s: %s\n",
				oFileName ? oFileName : "stdout", strerror(errno));
		exit(1);
	}

	if (blockSize) {
		iBlockSize = oBlockSize = blockSize;
		if (convertFunc == nullFunc)
		  ++flag;
	}
	if (iBlockSize == 0) {
		fprintf(stderr, "dd: ibs cannot be zero\n");
		exit(1);
	}
	if (oBlockSize == 0) {
		fprintf(stderr, "dd: obs cannot be zero\n");
		exit(1);
	}

	if ((iBuf = sbrk(iBlockSize)) == (char *) -1) {
		fprintf(stderr, "dd: not enough memory\n");
		exit(1);
	}
	if ((oBuf = (flag ? iBuf : sbrk(oBlockSize))) == (char *) -1) {
		fprintf(stderr, "dd: not enough memory\n");
		exit(1);
	}
	iBufCount = oBufCount = 0;
	oPtr = oBuf;

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	  signal(SIGINT, over);

	if (skipBlocks != 0) {
		struct stat st;
		if (fstat(iFd, &st) < 0 || 
				! (S_ISREG(st.st_mode) || S_ISBLK(st.st_mode)) ||
				lseek(iFd, (off_t) iBlockSize * (off_t) skipBlocks, SEEK_SET) == (off_t) -1) {
			do {
				if (read(iFd, iBuf, iBlockSize) == -1) {
					fprintf(stderr, "dd: Error skipping input: %s\n", strerror(errno));
					exit(1);
				}
			} while (--skipBlocks != 0);
		}
	}
	if (seekBlocks != 0) {
		if (lseek(oFd, (off_t) oBlockSize * (off_t) seekBlocks, SEEK_SET) == (off_t) -1) {
			fprintf(stderr, "dd: Seeking on output failed: %s\n", strerror(errno));
			exit(1);
		}
	}

outputAll:
	if (iBufCount-- == 0) {
		iBufCount = 0;
		if (countBlocks == 0 || (nInFull + nInPartial) != countBlocks) {
			if (convFlag & (NOERROR | SYNC)) {
				/* Reset for ignoring error or padding */
				for (iPtr = iBuf + iBlockSize; iPtr > iBuf; ) {
					*--iPtr = 0;
				}
			}
			iBufCount = read(iFd, iBuf, iBlockSize);
		}
		if (iBufCount == -1) {
			fprintf(stderr, "dd: Read error: %s\n", strerror(errno));
			if ((convFlag & NOERROR) == 0) {
				putOut();
				over(0);
			}
			iBufCount = 0;
			for (i = 0; i < iBlockSize; ++i) {
				if (iBuf[i] != 0)
				  iBufCount = i;
			}
			statistics();
		}
		if (iBufCount == 0 && --files <= 0) {
			putOut();
			over(0);
		}
		if (iBufCount != iBlockSize) {
			++nInPartial;
			if (convFlag & SYNC)	/* Pad input block with NULs to ibs-size */
			  iBufCount = iBlockSize;
		} else {
			++nInFull;
		}
		iPtr = iBuf;
		i = iBufCount >> 1;
		if ((convFlag & SWAB) && i) {
			int temp;
			do {
				temp = *iPtr++;
				iPtr[-1] = *iPtr;
				*iPtr++ = temp;
			} while (--i);
		}
		iPtr = iBuf;
		if (flag) {
			oBufCount = iBufCount;
			putOut();
			iBufCount = 0;
		}
		goto outputAll;
	}
	i = *iPtr++ & 0377;
	(*convertFunc)(i);
	goto outputAll;
}








