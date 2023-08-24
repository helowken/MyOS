#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <minix/config.h>
#include <minix/const.h>
#include <minix/minlib.h>

#define LINE_LEN	256
#define NR_ALREADY	512

typedef struct Already {
	struct Already *al_next;
	int al_dev;
	ino_t al_inum;
	nlink_t al_nlink;
} Already;

static char *prog;
static int silent = 0;		/* Silent mode */
static int all = 0;			/* All directory entries mode */
static int crossCheck = 0;	/* Do not cross device boundaries mode */
static char *startDir = ".";	/* Starting from here */
static int levels = 20000;	/* # of directory levels to print */
static Already *already[NR_ALREADY];

static int getBlockSize(char *dir, struct stat *st) {
	struct statfs stfs;
	static int fsBlockSize = -1, fsDev = -1;
	int d;

	if (st->st_dev == fsDev)
	  return fsBlockSize;

	if ((d = open(dir, O_RDONLY)) < 0) {
		perror(dir);
		return 0;
	}

	if (fstatfs(d, &stfs) < 0) {
		perror(dir);
		return 0;
	}

	fsBlockSize = stfs.f_bsize;
	fsDev = st->st_dev;

	return fsBlockSize;
}

int makeDName(char *d, char *f, char *out, int outLen) {
	char *cp;
	int length;

	length = strlen(f);
	if (strlen(d) + length + 2 > outLen)
	  return 0;
	for (cp = out; *d; *cp++ = *d++) {
	}
	if (*(cp - 1) != '/')
	  *cp++ = '/';
	while (length--) {
		*cp++ = *f++;
	}
	*cp = 0;
	return 1;
}

int done(dev_t dev, ino_t iNum, nlink_t nlink) {
/* done - have we encountered (dev, iNum) before? Returns a for yes,
 * 0 for no, and remembers (dev, iNum, nlink).
 */
	register Already **pap, *ap;

	pap = &already[(unsigned) (iNum % NR_ALREADY)];
	while ((ap = *pap) != NULL) {
		if (ap->al_inum == iNum && ap->al_dev == dev) {
			if (--ap->al_nlink == 0) {
				*pap = ap->al_next;
				free(ap);
			}
			return 1;
		}
		pap = &ap->al_next;
	}
	if ((ap = malloc(sizeof(*ap))) == NULL) {
		fprintf(stderr, "du: Out of memory\n");
		exit(1);
	}
	ap->al_next = NULL;
	ap->al_inum = iNum;
	ap->al_dev = dev;
	ap->al_nlink = nlink - 1;
	*pap = ap;
	return 0;
}

static long doDir(char *d, int thisLevel, dev_t dev) {
/* Process the directory d. Return the long size (in blocks) of d
 * and its descendants.
 */
	int maybePrint;
	struct stat st;
	long totalKB;
	char dent[LINE_LEN];
	DIR *dp;
	struct dirent *entry;
	int blockSize;

	if (lstat(d, &st) < 0) {
		fprintf(stderr, "%s: %s: %s\n", prog, d, strerror(errno));
		return 0L;
	}
	if (st.st_dev != dev && dev != 0 && crossCheck)
	  return 0;
	blockSize = getBlockSize(d, &st);
	if (blockSize < 1) {
		fprintf(stderr, "%s: %s: funny block size found (%d)\n",
				prog, d, blockSize);
		return 0;
	}
	totalKB = ((st.st_size + (blockSize - 1)) / blockSize) * blockSize / 1024;
	switch (st.st_mode & S_IFMT) {
		case S_IFDIR:
			/* Directories should not be linked except to "." and "..", so this
			 * directory should not already have been done.
			 */
			maybePrint = ! silent;
			if ((dp = opendir(d)) == NULL)
			  break;
			while ((entry = readdir(dp)) != NULL) {
				if (strcmp(entry->d_name, ".") == 0 ||
					strcmp(entry->d_name, "..") == 0)
				  continue;
				if (! makeDName(d, entry->d_name, dent, sizeof(dent)))
				  continue;
				totalKB += doDir(dent, thisLevel - 1, st.st_dev);
			}
			closedir(dp);
			break;
		case S_IFBLK:
		case S_IFCHR:
			/* st_size for special files is not related to blocks used. */
			totalKB = 0;
			/* Fall through */
		default:
			if (st.st_nlink > 1 && done(st.st_dev, st.st_ino, st.st_nlink))
			  return 0L;
			maybePrint = all;
			break;
	}
	if (thisLevel >= levels || (maybePrint && thisLevel >= 0)) 
	  printf("%ld\t%s\n", totalKB, d);

	return totalKB;
}

int main(int argc, char **argv) {
	int c;

	prog = getProg(argv);
	while ((c = getopt(argc, argv, "asxdl:")) != EOF) {
		switch (c) {
			case 'a': all = 1; break;
			case 's': silent = 1; break;
			case 'x':
			case 'd': crossCheck = 1; break;
			case 'l': levels = atoi(optarg); break;
			default:
				fprintf(stderr, 
					"Usage: %s [-asx] [-l levels] [startdir]\n", prog);
				exit(1);
		}
	}

	do {
		if (optind < argc)
		  startDir = argv[optind++];
		doDir(startDir, levels, 0);
	} while (optind < argc);
	return 0;
}
