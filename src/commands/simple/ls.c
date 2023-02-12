#define nil 0
#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "stdlib.h"
#include "unistd.h"
#include "dirent.h"
#include "time.h"
#include "pwd.h"
#include "grp.h"
#include "errno.h"
#include "fcntl.h"
#include "limits.h"
#include "termios.h"
#include "sys/ioctl.h"

typedef struct File {
	struct File *next;		
	char *name;
	ino_t ino;
	mode_t mode;
	uid_t uid;
	gid_t gid;
	nlink_t nlink;
	dev_t rdev;
	off_t size;
	time_t mtime;
	time_t atime;
	time_t ctime;
} File;

typedef enum { SURFACE, SURFACE_1, SUBMERGED } Depth;
typedef enum {BOTTOM, SINKING, FLOATING } State;

static char ifmtStr[] = "0pcCd?bB-?l?s???";
#define ifmt(mode)	ifmtStr[((mode) >> 12) & 0xF]

static char allowed[] = "acdfghilnpqrstu1ACDFLMRTX";
static char flags[sizeof(allowed)];
static char arg0Flag[] = "cdfmrtx";		/* These in argv[0] goto upper case. */
static char *prog;			/* Last component of argv[0] */
static int uid, gid;
static int exitStatus;
static int istty;			/* Output is on a terminal. */

/* Some terminals ignore more than 80 characters on a line. Dumb ones wrap
 * when the cursor hits the side. Nice terminals don't wrap until they have
 * to print the 81st character. whether we like it or not, no column 80.
 */
static int nCols = 79;

#define NSEP		3		/* # spaces between columns */
#define MAX_COLS	128		/* Max # of files per line. */

static int field = 0;		/* (Used to be) Fields that must be printed. */
							/* (now) Effects triggered by certain flags. */

#define L_INODE		0x0001	/* -i */
#define L_BLOCKS	0x0002	/* -s */
#define L_EXTRA		0x0004	/* -X */
#define L_MODE		0x0008	/* -lMX */
#define L_LONG		0x0010	/* -l */
#define L_GROUP		0x0020	/* -g */
#define L_BYTIME	0x0040	/* -tuc */
#define L_ATIME		0x0080	/* -u */
#define L_CTIME		0x0100	/* -c */
#define L_MARK		0x0200	/* -F */
#define L_MARKDIR	0x0400	/* -p */
#define L_TYPE		0x0800	/* -D */
#define L_LONGTIME	0x1000	/* -T */
#define L_DIR		0x2000	/* -d */
#define L_KMG		0x4000	/* -h */

int (*status)(const char *file, struct stat *stp);

static void reportErr(char *label) {
	fprintf(stderr, "%s: %s: %s\n", prog, label, strerror(errno));
	exitStatus = 1;
}

static void setStat(File *f, struct stat *stp) {
	f->ino = stp->st_ino;
	f->mode = stp->st_mode;
	f->nlink = stp->st_nlink;
	f->uid = stp->st_uid;
	f->gid = stp->st_gid;
	f->rdev = stp->st_rdev;
	f->size = stp->st_size;
	f->mtime = stp->st_mtime;
	f->atime = stp->st_atime;
	f->ctime = stp->st_ctime;
}

static void setFlags(char *fls) {
	int c;

	while ((c = *fls++) != 0) {
		if (strchr(allowed, c) == nil) {
			fprintf(stderr, "Usage: %s [-%s] [file ...]\n", prog, allowed);
			exit(1);
		} else if (strchr(flags, c) == nil) {
			flags[strlen(flags)] = c;
		}
	}
}

static int present(int f) {
	return f == 0 || strchr(flags, f) != nil;
}

static void checkSetFlag(char f, int fv) {
	if (present(f))
	  field |= fv;
}

static void heapErr() {
	fprintf(stderr, "%s: Out of memory\n", prog);
	exit(-1);
}

static void *allocate(size_t n) {
	void *a;

	if ((a = malloc(n)) == nil)
	  heapErr();
	return a;
}

void *reallocate(void *a, size_t n) {
	if ((a = realloc(a, n)) == NULL)
	  heapErr();
	return a;
}

static File *newFile(char *name) {
/* Create file structure for given name. */
	File *new;

	new = (File *) allocate(sizeof(*new));
	new->name = strcpy((char *) allocate(strlen(name) + 1), name);
	return new;
}

static void pushFile(File **aFileList, File *new) {
/* Add file to the head of a list. */
	new->next = *aFileList;
	*aFileList = new;
}

/* Path name construction, addPath adds a component, delPath removes it.
 * The string path is used throughout the program as the file under examination.
 */
static char *path;	/* Path name constructed in path[]. */
static int pLen = 0, pIdx = 0;	/* Length/index for path[]. */

static void addPath(int *delIdx, char *name) {
/* Add a component to path. (name may also be a full path at the first call)
 * The index where the current path ends is stored in *pIdx.
 */
	if (pLen == 0)
	  path = (char *) allocate((pLen = 32) * sizeof(path[0]));
	if (pIdx == 1 && path[0] == '.')
	  pIdx = 0;		/* Remove "." */
	
	*delIdx = pIdx;	/* Record point to go back to for delPath. */
	
	if (pIdx > 0 && path[pIdx - 1] != '/')
	  path[pIdx++] = '/';

	do {
		if (*name != '/' || pIdx == 0 || path[pIdx - 1] != '/') {
			if (pIdx == pLen) 
			  path = (char *) reallocate((void *) path, (pLen *= 2) * sizeof(path[0]));
			path[pIdx++] = *name;
		}
	} while (*name++ != 0);

	--pIdx;		/* Put pIdx back at the null. The path[pIdx++] = '/'
				   statement will overwrite it at the next call. */
}

#define delPath(delIdx)	(path[(pIdx = delIdx)] = 0)	/* Remove component. */

static File *popFile(File **fileList) {
/* Pop file off top of file list. */
	File *f;

	f = *fileList;
	*fileList = f->next;
	return f;
}

static void delFile(File *old) {
	free((void *) old->name);
	free((void *) old);
}

static int (*CMP)(File *f1, File *f2);
static int (*rCMP)(File *f1, File *f2);

static int nameCmp(File *f1, File *f2) {
	return strcmp(f1->name, f2->name);
}

static int mtimeCmp(File *f1, File *f2) {
	return f1->mtime == f2->mtime ? 0 : f1->mtime > f2->mtime  ? -1 : 1;
}

static int atimeCmp(File *f1, File *f2) {
	return f1->atime == f2->atime ? 0 : f1->atime > f2->atime  ? -1 : 1;
}

static int ctimeCmp(File *f1, File *f2) {
	return f1->ctime == f2->ctime ? 0 : f1->ctime > f2->ctime  ? -1 : 1;
}

static int typeCmp(File *f1, File *f2) {
	return ifmt(f1->mode) - ifmt(f2->mode);
}

static int revCmp(File *f1, File *f2) {
	return (*rCMP)(f2, f1);
}

static void mergeSort(File **al) {
/* This is either a stable mergeSort, or thermal noise, I'm no longer sure.
 * It must be called like this: if (L != nil && L->next != nil) mergeSort(&L);
 */
	File *l1, **mid;	
	File *l2;

	l1 = *(mid = &(*al)->next);
	do {
		if ((l1 = l1->next) == nil)
		  break;
		mid = &(*mid)->next;
	} while ((l1 = l1->next) != nil);

	l2 = *mid;
	*mid = nil;

	if ((*al)->next != nil)
	  mergeSort(al);
	if (l2->next != nil)
	  mergeSort(&l2);

	l1 = *al;
	for (;;) {
		if ((*CMP)(l1, l2) <= 0) {
			if ((l1 = *(al = &l1->next)) == nil) {
				*al = l2;
				break;
			}
		} else {
			*al = l2;
			l2 = *(al = &l2->next);
			*al = l1;
			if (l2 == nil)
			  break;
		}
	}
}

static void sort(File **al) {
/* Sort the files according to the flags. */	
	if (! present('f') && *al != nil && (*al)->next != nil) {
		CMP = nameCmp;	

		if (! (field & L_BYTIME)) {
			/* Sort on name */
			if (present('r')) {
				rCMP = CMP;
				CMP = revCmp;
			}
			mergeSort(al);
		} else {
			/* Sort on name first, then sort on time. */
			mergeSort(al);
			if (field & L_CTIME)
			  CMP = ctimeCmp;
			else if (field & L_ATIME)
			  CMP = atimeCmp;
			else
			  CMP = mtimeCmp;

			if (present('r')) {
				rCMP = CMP;
				CMP = revCmp;
			}
			mergeSort(al);
		}

		/* Separate by file type if so desired. */
		if (field & L_TYPE) {
			CMP = typeCmp;
			mergeSort(al);
		}
	}
}

/* Transform size of file to number of blocks. This was once a function that
 * guessed the number of indirect blocks, but that nonsense has been removed.
 */
#define BLOCK			512
#define numBlocks(f)	(((f)->size + BLOCK - 1) / BLOCK)

/* From number of blocks to kilobytes. */
#define numBlk2K(nb)	(((nb) + (1024 / BLOCK - 1)) / (1024 / BLOCK))

static off_t countBlocks(File *fileList) {
/* Compute total block count for a list of files. */
	off_t cb = 0;

	while (fileList != nil) {
		switch (fileList->mode & S_IFMT) {
			case S_IFDIR:
			case S_IFREG:
			case S_IFLNK:
				cb += numBlocks(fileList);
		}
		fileList = fileList->next;
	}
	return cb;
}

static int dotFlag(char *name) {
/* Return flag that would make ls list this name: -a or -A. */
	if (*name++ != '.')
	  return 0;

	switch (*name++) {
		case 0:
			return 'a';		/* "." */
		case '.':
			if (*name == 0)
			  return 'a';	/* ".." */
		default:
			return 'A';		/* ".*" */
	}
}

static int addDir(File **aFileList, char *name) {
/* Add directory entries of directory name to a file list. */
	DIR *dp;
	struct dirent *entry;

	if (access(name, F_OK) < 0) {
		reportErr(name);
		return 0;
	}

	if ((dp = opendir(name)) == nil) {
		reportErr(name);
		return 0;
	}
	while ((entry = readdir(dp)) != nil) {
		if (entry->d_ino != 0 && present(dotFlag(entry->d_name))) {
			pushFile(aFileList, newFile(entry->d_name));
			aFileList = &(*aFileList)->next;
		}
	}
	closedir(dp);
	return 1;
}

static File *fileCol[MAX_COLS];	/* fileCol[i] is list of files for column i. */
static int nFiles;	/* # files to print */
static int nLines;	/* # of lines needed. */

static void columnise(File *fileList, int nPerLine) {
/* Chop list of files up in columns. Note that 3 columns are used for 5 files
 * even though nPerLine may be 4, fileCol[3] will simple be nil.
 */
	int i, j;

	nLines = (nFiles + nPerLine - 1) / nPerLine;	/* nLines needed for nFiles */
	fileCol[0] = fileList;

	for (i = 1; i < nPerLine; ++i) {	/* Give nLines files to each column. */
		for (j = 0; j < nLines && fileList != nil; ++j) {
			fileList = fileList->next;
		}
		fileCol[i] = fileList;
	}
}

/* Width of entire column, and of several fields. */
enum { W_COL, W_INO, W_BLK, W_NLINK, W_UID, W_GID, W_SIZE, W_NAME, MAX_FIELDS };

static unsigned char fieldWidth[MAX_COLS][MAX_FIELDS];

static int nSpace = 0;		/* This many spaces have not been printed yet. */
#define spaces(n)	(nSpace = (n))
#define terpri()	(nSpace = 0, putchar('\n'))		/* No trailing spaces */

static void maxise(unsigned char *aw, int w) {
/* Set *aw to the larger of it and w. */
	if (w > *aw) {
		if (w > UCHAR_MAX)
		  w = UCHAR_MAX;
		*aw = w;
	}
}

static int numWidth(unsigned long n) {
/* Compute width of 'n' when printed. */
	int width = 0;

	do {
		++width;
	} while ((n /= 10) > 0);
	return width;
}

static void numeral(int i, char **pp) {
	char itoa[3 * sizeof(int)], *a = itoa;

	do {
		*a++ = i % 10 + '0';
	} while ((i /= 10) > 0);

	do {
		*(*pp)++ = *--a;
	} while (a > itoa);
}

#define K	1024L	/* A kilobyte counts in multiples of K */
#define T	1000L	/* A megabyte in T*K, a gigabyte in T*T*K */

static char *cxsize(File *f) {
/* Try and fail to turn a 32 bit size into 4 readable characters. */
	static char size[] = "1.2m";
	char *p = size;
	off_t z;

	size[1] = size[2] = size[3] = 0;

	if (f->size <= 5 * K) {	/* <= 5K prints as is. */
		numeral((int) f->size, &p);
		return size;
	}
	z = (f->size + K - 1) / K;

	if (z <= 999) {		/* Print as 123k. */
		numeral((int) z, &p);
		*p = 'k';	/* Can't use 'K', looks bad */
	} else if (z * 10 <= 99 * T) {	/* 1.2m (Try ls -X /dev/at0 */
		z = (z * 10 + T - 1) / T;
		numeral((int) z / 10, &p);
		*p++ = '.';
		numeral((int) z % 10, &p);
		*p = 'm';
	} else if (z <= 999 * T) {	/* 123m */
		numeral((int) ((z + T - 1) / T), &p);
		*p = 'm';
	} else {	/* 1.2g */
		z = (z * 10 + T * T -1) / (T * T);
		numeral((int) z / 10, &p);
		*p++ = '.';
		numeral((int) z % 10, &p);
		*p = 'g';
	}
	return size;
}

/* Two functions, uidName and gidName, translate id's to readable names.
 * All names are remembered to avoid searching the password file.
 */
#define NUM_NAMES	(1 << (sizeof(int) + sizeof(char *)))
typedef enum { PASSWD, GROUP } WhatMap;

#define uidName(uid)	idName((uid), PASSWD)
#define gidName(gid)	idName((gid), GROUP)

typedef struct IdName {	/* Hash list of names. */
	struct IdName *next;
	char *name;
	uid_t id;
} IdName;

static IdName *uids[NUM_NAMES], *gids[NUM_NAMES];

static char *idName(unsigned id, WhatMap map) {
/* Return name for a given user/group id. */
	IdName *i;
	IdName **ids = &(map == PASSWD ? uids : gids)[id % NUM_NAMES];

	while ((i = *ids) != nil && id < i->id) {
		ids = &i->next;
	}

	if (i == nil || id != i->id) {
		/* Not found, go look in the password or group map. */
		char *name = nil;
		char noName[3 * sizeof(uid_t)];

		if (! present('n')) {
			if (map == PASSWD) {
				struct passwd *pw = getpwuid(id);

				if (pw != nil)
				  name = pw->pw_name;
			} else {
				struct group *gr = getgrgid(id);

				if (gr != nil)
				  name = gr->gr_name;
			}
		}
		if (name == nil) {
			/* Can't find it, weird. Use numerical "name." */
			sprintf(noName, "%u", id);
			name = noName;
		}

		/* Add a new id-to-name cell. */
		i = allocate(sizeof(*i));
		i->id = id;
		i->name = allocate(strlen(name) + 1);
		strcpy(i->name, name);
		i->next = *ids;
		*ids = i;
	}
	return i->name;
}

#define PAST	(26 * 7 * 24 * 3600L) /* Half a year ago. */
/* Between PAST and FUTURE from now a time is printed otherwise a year. */
#define FUTURE	(1 * 7 * 24 * 3600)		/* One week. */

static char *timestamp(File *f) {
/* Translate the right time field into something readable. */
	struct tm *tm;
	time_t t;
	static time_t now;
	static int drift = 0;
	static char date[] = "Jan 19 03:14:07 2038";
	static char month[] = "JanFebMrAprMayJunJulAugSepOctNovDec";

	t = f->mtime;
	if (field & L_ATIME)
	  t = f->atime;
	if (field & L_CTIME)
	  t = f->ctime;

	tm = localtime(&t);
	if (--drift < 0) {
		time(&now);
		drift = 50;		/* Limit time() calls */
	}

	if (field & L_LONGTIME) 
	  sprintf(date, "%.3s %2d %02d:%2d:%2d %d",
			  month + 3 * tm->tm_mon,
			  tm->tm_mday,
			  tm->tm_hour, tm->tm_min, tm->tm_sec,
			  1900 + tm->tm_year);
	else if (t < now - PAST || t > now + FUTURE) 
	  sprintf(date, "%.3s %2d  %d",
			  month + 3 * tm->tm_mon,
			  tm->tm_mday,
			  1900 + tm->tm_year);
	else 
	  sprintf(date, "%.3s %2d %02d:%02d",
			  month + 3 * tm->tm_mon,
			  tm->tm_mday,
			  tm->tm_hour, tm->tm_min);

	return date;
}

static void printName(char *name) {
/* Print a name with control characters as '?' (unless -q). The terminal is
 * assumed to be eight bit clean.
 */
	int c, q = present('q');

	while ((c = (unsigned char) *name++) != 0) {
		if (q && (c < ' ' || c == 0177))
		  c = '?';
		putchar(c);
	}
}

static int mark(File *f, int doIt) {
	int c;

	c = 0;
	if (field & L_MARK) {
		switch (f->mode & S_IFMT) {
			case S_IFDIR:
				c = '/';
				break;
			case S_IFIFO:
				c = '|';
				break;
			case S_IFLNK:
				c = '@';
				break;
			case S_IFREG:
				if (f->mode & (S_IXUSR | S_IXGRP | S_IXOTH)) 
				  c = '*';
				break;
		}
	} else if (field & L_MARKDIR) {
		if (S_ISDIR(f->mode))
		  c = '/';
	}

	if (doIt && c != 0)
	  putchar(c);
	return c;
}

static char *permissions(File *f) {
/* Compute long or short rwx bits. */
	static char rwx[] = "drwxr-x--x";

	rwx[0] = ifmt(f->mode);
	/* Note that rwx[0] is a guess for the more alien file types. It is
	 * correct for BSD4.3 and derived systems. I just don't know how
	 * "standardized" these numbers are.
	 */
	if (field & L_EXTRA) {	/* Short style */
		int mode = f->mode, ucase = 0;

		if (uid == f->uid) {	/* What group of bits to use. */
			ucase = (mode << 3) | (mode << 6);
			/* Remember if group or others have permissions. */
		} else if (gid == f->gid) 
		  mode <<= 3;
		else 
		  mode <<= 6;

		rwx[1] = mode & S_IRUSR ? (ucase & S_IRUSR ? 'R' : 'r') : '-';
		rwx[2] = mode & S_IWUSR ? (ucase & S_IWUSR ? 'W' : 'w') : '-';

		if (mode & S_IXUSR) {
			static char sbit[] = { 'x', 'g', 'u', 's' };

			rwx[3] = sbit[(f->mode & (S_ISUID | S_ISGID)) >> 10];
			if (ucase & S_IXUSR)
			  rwx[3] += 'A' - 'a';
		} else { 
			rwx[3] = f->mode & (S_ISUID | S_ISGID) ? '=' : '-';
		}
		rwx[4] = 0;
	} else {	/* Long form */
		char *p = rwx + 1;
		int mode = f->mode;

		do {
			p[0] = (mode & S_IRUSR) ? 'r' : '-';
			p[1] = (mode & S_IWUSR) ? 'w' : '-';
			p[2] = (mode & S_IXUSR) ? 'x' : '-';
			mode <<= 3;
		} while ((p += 3) <= rwx + 7);

		if (f->mode & S_ISUID)
		  rwx[3] = f->mode & S_IXUSR ? 's' : '=';
		if (f->mode & S_ISGID)
		  rwx[6] = f->mode & S_IXGRP ? 's' : '=';
		if (f->mode & S_ISVTX)
		  rwx[9] = f->mode & S_IXOTH ? 't' : '=';
	}
	return rwx;
}

static void doPrint(File *f, int col, int doIt) {
/* Either compute the number of spaces needed to print file f (doIt == 0) or
 * really print it (doIt == 1).
 */
	int width = 0, n;
	char *p;
	unsigned char *fldWidth = fieldWidth[col];

	while (nSpace > 0) {
		putchar(' ');
		--nSpace;	/* Fill gap between two columns */
	}

	if (field & L_INODE) {
		if (doIt) 
		  printf("%*d ", fldWidth[W_INO], f->ino);
		else {
			maxise(&fldWidth[W_INO], numWidth(f->ino));
			++width;
		}
	}
	if (field & L_BLOCKS) {
		unsigned long nb = numBlk2K(numBlocks(f));
		if (doIt) 
		  printf("%*lu ", fldWidth[W_BLK], nb);
		else {
			maxise(&fldWidth[W_BLK], numWidth(nb));
			++width;
		}
	}
	if (field & L_MODE) {
		if (doIt) 
		  printf("%s ", permissions(f));
		else
		  width += (field & L_EXTRA) ? 5 : 11;
	}
	if (field & L_EXTRA) {
		p = cxsize(f);
		n = strlen(p) + 1;

		if (doIt) {
			n = fldWidth[W_SIZE] - n;
			while (n > 0) {
				putchar(' ');
				--n;
			}
			printf("%s ", p);
		} else {
			maxise(&fldWidth[W_SIZE], n);
		}
	}
	if (field & L_LONG) {
		if (doIt) 
		  printf("%*u ", fldWidth[W_NLINK], (unsigned) f->nlink);
		else {
			maxise(&fldWidth[W_NLINK], numWidth(f->nlink));
			++width;
		}
		if (! (field & L_GROUP)) {
			if (doIt) 
			  printf("%-*s  ", fldWidth[W_UID], uidName(f->uid));
			else {
				maxise(&fldWidth[W_UID], strlen(uidName(f->uid)));
				width += 2;
			}
		}
		if (doIt) 
		  printf("%-*s  ", fldWidth[W_GID], gidName(f->gid));
		else {
			maxise(&fldWidth[W_GID], strlen(gidName(f->gid)));
			width += 2;
		}

		switch (f->mode & S_IFMT) {
			case S_IFBLK:
			case S_IFCHR:
				if (doIt) 
				  printf("%*lX ", fldWidth[W_SIZE], (unsigned long) f->rdev);
				else {
					maxise(&fldWidth[W_SIZE], numWidth(f->rdev));
					++width;
				}
				break;
			default:
				if (field & L_KMG) {
					p = cxsize(f);
					n = strlen(p) + 1;

					if (doIt) {
						n = fldWidth[W_SIZE] - n;
						while (n > 0) {
							putchar(' ');
							--n;
						}
						printf("%s ", p);
					} else 
					  maxise(&fldWidth[W_SIZE], n);
				} else {
					if (doIt)
					  printf("%*lu ", fldWidth[W_SIZE], (unsigned long) f->size);
					else {
						maxise(&fldWidth[W_SIZE], numWidth(f->size));
						++width;
					}
				}
		}

		if (doIt) 
		  printf("%s ", timestamp(f));
		else 
		  width += (field & L_LONGTIME) ? 21 : 13;
	}

	n = strlen(f->name);
	if (doIt) {
		printName(f->name);
		if (mark(f, 1) != 0)
		  ++n;

		if ((field & L_LONG) && S_ISLNK(f->mode)) {
			char *buf;
			int r, delIdx;

			buf = (char *) allocate(((size_t) f->size + 1) * sizeof(buf[0]));
			addPath(&delIdx, f->name);
			r = readlink(path, buf, (int) f->size);
			delPath(delIdx);
			if (r > 0)
			  buf[r] = 0;
			else {
				r = 1;
				strcpy(buf, "?");
			}
			printf(" -> ");
			printName(buf);
			free((void *) buf);
			n += 4 + r;
		}
		spaces(fldWidth[W_NAME] - n);
	} else {
		if (mark(f, 0) != 0)
		  ++n;
		if ((field & L_LONG) && S_ISLNK(f->mode)) 
		  n += 4 + (int) f->size;
		maxise(&fldWidth[W_NAME], n + NSEP);

		for (n = 1; n < MAX_FIELDS; ++n) {
			width += fldWidth[n];
		}
		maxise(&fldWidth[W_COL], width);
	}
}

static int print(File *fileList, int nPerLine, int doIt) {
/* Try (doIt == 0), or really print the list of files over nPerLine columns.
 * Return true if it can be done in nPerLine columns or if nPerLine == 1.
 */
	register File *f;
	register int col, fld, totalLen;

	columnise(fileList, nPerLine);

	if (! doIt) {
		for (col = 0; col < nPerLine; ++col) {
			for (fld = 0; fld < MAX_FIELDS; ++fld) {
				fieldWidth[col][fld] = 0;
			}
		}
	}

	while (--nLines >= 0) {
		totalLen = 0;

		for (col = 0; col < nPerLine; ++col) {
			if ((f = fileCol[col]) != nil) {
				fileCol[col] = f->next;
				doPrint(f, col, doIt);
			}
			if (! doIt && nPerLine > 1) {
				/* See if this line is not too long. */
				if (fieldWidth[col][W_COL] == UCHAR_MAX) 
				  return 0;
				totalLen += fieldWidth[col][W_COL];
				if (totalLen > nCols + NSEP)  
				  return 0;
			}
		}
		if (doIt)
		  terpri();
	}
	return 1;
}

static int countFiles(File *fileList) {
	int n = 0;

	while (fileList != nil) {
		++n;
		fileList = fileList->next;
	}
	return n;
}

static void listFiles(File *fileList, Depth depth, State state) {
	File *dirList = nil, **aFileList = &fileList, **aDirList = &dirList;
	int nPerLine;
	static int white = 1;	/* Nothing printed yet. */

	/* Flush every thing previously printed, so new error output will
	 * not intermix with files listed earlier.
	 */
	fflush(stdout);

	if (field != 0 || state != BOTTOM) {	/* Need stat(2) info. */
		while (*aFileList != nil) {
			static struct stat st;
			int r, delIdx;

			addPath(&delIdx, (*aFileList)->name);
			if ((r = status(path, &st)) < 0 && 
					(status == lstat || lstat(path, &st) < 0)) {
				if (depth != SUBMERGED || errno != ENOENT)
					reportErr((*aFileList)->name);
				delFile(popFile(aFileList));
			} else {
				setStat(*aFileList, &st);
				aFileList = &(*aFileList)->next;
			}
			delPath(delIdx);
		}
	}
	sort(&fileList);

	if (depth == SUBMERGED && (field & (L_BLOCKS | L_LONG))) 
	  printf("total %ld\n", numBlk2K(countBlocks(fileList)));

	if (state == SINKING || depth == SURFACE_1) {
		/* Don't list directories themselves, list their contents later. */
		aFileList = &fileList;
		while (*aFileList != nil) {
			if (S_ISDIR((*aFileList)->mode)) {
				pushFile(aDirList, popFile(aFileList));
				aDirList = &(*aDirList)->next;
			} else 
			  aFileList = &(*aFileList)->next;
		}
	}

	if ((nFiles = countFiles(fileList)) > 0) {
		/* Print files in how many columns? */
		nPerLine = ! present('C') ? 1 : nFiles < MAX_COLS ? nFiles : MAX_COLS;

		while (! print(fileList, nPerLine, 0)) {	/* Try first */
			--nPerLine;	
		}
		print(fileList, nPerLine, 1);	/* Then do it! */
		white = 0;
	}

	while (fileList != nil) {	/* Destroy file list */
		if (state == FLOATING && S_ISDIR(fileList->mode)) {
			/* But keep these directories for ls -R. */
			pushFile(aDirList, popFile(&fileList));
			aDirList = &(*aDirList)->next;
		} else 
		  delFile(popFile(&fileList));
	}

	while (dirList != nil) {	/* List directories */
		if (dotFlag(dirList->name) != 'a' || depth != SUBMERGED) {
			int delIdx;

			addPath(&delIdx, dirList->name);
			fileList = nil;
			if (addDir(&fileList, path)) {
				if (depth != SURFACE_1) {
					if (! white) 
					  putchar('\n');
					printf("%s:\n", path);
					white = 0;
				}
				listFiles(fileList, SUBMERGED,
						state == FLOATING ? FLOATING : BOTTOM);
			}
			delPath(delIdx);
		}
		delFile(popFile(&dirList));
	}
}

int main(int argc, char **argv) {
	File *fileList = nil, **aFileList = &fileList;
	Depth depth;
	char *lsFlags;
	WinSize ws;

	uid = geteuid();
	gid = getegid();
	
	if ((prog = strrchr(argv[0], '/')) == nil)
	  prog = argv[0];
	else
	  ++prog;
	++argv;

	if (strcmp(prog, "ls") != 0) {
		char *p = prog + 1;

		while (*p != 0) {
			if (strchr(arg0Flag, *p) != nil)
			  *p += 'A' - 'a';
			++p;
		}
		setFlags(prog + 1);
	}
	while (*argv != nil && (*argv)[0] == '-') {
		if ((*argv)[1] == '-' && (*argv)[2] == 0) {
			++argv;
			break;
		}
		setFlags(*argv++ + 1);
	}

	istty = isatty(1);

	if (istty && (lsFlags = getenv("LSOPTS")) != nil) {
		if (*lsFlags == '-')
		  ++lsFlags;
		setFlags(lsFlags);
	}

	if (! present('1') && ! present('C') && ! present('l') &&
			(istty || present('M') || present('X') || present('F')))
	  setFlags("C");

	if (istty)
	  setFlags("q");
	
	if (uid == 0 || present('a'))
	  setFlags("A");

	checkSetFlag('i', L_INODE);
	checkSetFlag('s', L_BLOCKS);
	checkSetFlag('M', L_MODE);
	checkSetFlag('X', L_EXTRA | L_MODE);
	checkSetFlag('t', L_BYTIME);
	checkSetFlag('u', L_ATIME);
	checkSetFlag('c', L_CTIME);
	checkSetFlag('l', L_MODE | L_LONG);
	checkSetFlag('g', L_MODE | L_LONG | L_GROUP);
	checkSetFlag('F', L_MARK);
	checkSetFlag('p', L_MARKDIR);
	checkSetFlag('D', L_TYPE);
	checkSetFlag('T', L_MODE | L_LONG | L_LONGTIME);
	checkSetFlag('d', L_DIR);
	checkSetFlag('h', L_KMG);
	if (field & L_LONG)
	  field &= ~L_EXTRA;

	status = present('L') ? stat : lstat;

	if (present('C')) {
		int t = istty ? STDOUT_FILENO : open("/dev/tty", O_RDONLY);

		if (t >= 0 && ioctl(t, TIOC_GET_WINSZ, &ws) == 0 && ws.ws_col > 0) 
		  nCols = ws.ws_col - 1;

		if (t != STDOUT_FILENO && t != -1)
		  close(t);
	}

	depth = SURFACE;

	if (*argv == nil) {
		if (! (field & L_DIR))
		  depth = SURFACE_1;
		pushFile(aFileList, newFile("."));
	} else {
		if (argv[1] == nil && ! (field & L_DIR))
		  depth = SURFACE_1;

		do {
			pushFile(aFileList, newFile(*argv++));
			aFileList = &(*aFileList)->next;
		} while (*argv != nil);
	}

	listFiles(fileList, depth,
		(field & L_DIR) ? BOTTOM : present('R') ? FLOATING : SINKING);

	return exitStatus;
}

