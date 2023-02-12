#define nil	0
#include "stdio.h"
#include "sys/types.h"
#include "stdlib.h"
#include "string.h"
#include "stddef.h"
#include "unistd.h"
#include "fcntl.h"
#include "time.h"
#include "sys/stat.h"
#include "utime.h"
#include "dirent.h"
#include "errno.h"
#include "sys/dir.h"
#include "limits.h"
#include "minix/const.h"

#define CHUNK	(1024 << (sizeof(int) + sizeof(char *)))

#ifndef CONFORMING
#define	CONFORMING	1	/* Precisely POSIX conforming. */
#endif

#define arraySize(a)	(sizeof(a) / sizeof((a)[0]))
#define arrayLimit(a)	((a) + arraySize(a))

#define PATH_NAME(pp)		( (pp)->path)
#define PATH_LENGTH(pp)		((pp)->idx)
#define PATH_DROP(pp)		deallocate((void *) (pp)->path)

#define exists(stp)		((stp)->st_ino != 0)
#define isDir(stp)		(exists(stp) && S_ISDIR((stp)->st_mode))

typedef struct {
	char *path;		/* The actual pathname. */
	size_t idx;		/* Index ofr the terminating null byte. */
	size_t limit;	/* Actual length of the path array. */
} PathName;

typedef struct EntryList {
	struct EntryList *next;
	char *name;
} EntryList;

typedef enum { CP, MV, RM, LN, CPDIR, CLONE } Identity;
typedef enum { COPY, MOVE, REMOVE, LINK } Action;

static char *prog;
static int exitCode = 0;
static Identity identity;	/* How did the user call me? */
static Action action;		/* Copying, moving, or linking. */
static int pflag = 0;		/* -p/-s: Make original and copy the same. */
static int iflag = 0;		/* -i: Interactive overwriting/deleting. */
static int fflag = 0;		/* -f: Force. */
static int sflag = 0;		/* -s: Make a symbolic link (ln/clone). */
static int Sflag = 0;		/* -S: Make symlink if across devices. */
static int mflag = 0;		/* -m: Merge trees, no target dir trickery. */
static int rflag = 0;		/* -r/R: Recursively copy a tree. */
static int vflag = 0;		/* -v: Verbose. */
static int xflag = 0;		/* -x: Don't traverse past mount points. */
static int xDev = 0;		/* Set when moving or linking cross-device. */
static int expand = 0;		/* Expand symlinks, ignore links. */
static int conforming = CONFORMING;	/* Sometimes standards are a pain. */

static int fcMask;			/* File creation mask. */
static int euid, egid;		/* Effective uid & gid. */
static int istty;			/* Can have terminal input. */

static ino_t topDstIno;
static dev_t topDstDev;
static dev_t topSrcDev;

static void do1(PathName *src, PathName *dst, int depth);

static void report(const char *label) {
	if (action == REMOVE && fflag)
	  return;
	fprintf(stderr, "%s: %s: %s\n", prog, label, strerror(errno));
	exitCode = 1;
}

static void report2(const char *src, const char *dst) {
	fprintf(stderr, "%s %s %s: %s\n", prog, src, dst, strerror(errno));
	exitCode = 1;
}

static void fatal(const char *label) {
	report(label);
	exit(1);
}

static void *allocate(void *mem, size_t size) {
/* Like realloc, but with checking of the return value. */
	if ((mem = mem == nil ? malloc(size) : realloc(mem, size)) == nil)
	  fatal("mallocate()");
	return mem;
}

static void deallocate(void *mem) {
	if (mem != nil)
	  free(mem);
}

static void pathInit(PathName *pp) {
/* Initialize a pathname to the null string. */
	pp->limit = DIRSIZ + 1;
	pp->path = allocate(nil, pp->limit);
	pp->idx = 0;
	pp->path[0] = 0;
}

static void pathAdd(PathName *pp, const char *name) {
/* Add a component to a pathname. */
	size_t limit;
	char *p;

	limit = pp->idx + strlen(name) + 2;

	if (limit > pp->limit) {
		limit += limit / 2;		/* Add an extra 50% growing space. */
		pp->limit = limit;
		pp->path = allocate(pp->path, limit);
	}
	
	p = pp->path + pp->idx;
	if (p > pp->path && p[-1] != '/')
	  *p++ = '/';
	
	while (*name != 0) {
		/* When there are more than 1 '/', just reserve 1. */
		if (*name != '/' || p == pp->path || p[-1] != '/') 
		  *p++ = *name;
		++name;
	}
	*p = 0;
	pp->idx = p - pp->path;
}

static void pathTrunc(PathName *pp, size_t delIdx) {
/* Delete part of a pathname to a remembered length. */
	pp->idx = delIdx;
	pp->path[delIdx] = 0;
}

static char *baseName(const char *path) {
/* Return the last component of a pathname. */
	const char *p = path;

	for (;;) {
		while (*p == '/') {		/* Trailing slashes? */
			++p;
		}
		if (*p == 0)
		  break;
		path = p;
		while (*p != 0 && *p != '/') {	/* Skip component. */
			++p;
		}
	}
	return (char *) path;
}

static int affirmative() {
/* Get a yes/no answer from the suspecting user. */
	int c;
	int ok;

	fflush(stdout);
	fflush(stderr);

	while ((c = getchar()) == ' ') {
	}
	ok = (c == 'y' || c == 'Y');
	while (c != EOF && c != '\n') {
		c = getchar();
	}
	return ok;
}

static int writable(const struct stat *stp) {
/* True iff the file with the given attributes allows writing. (And we have
 * a terminal to ask if ok to overwrite. )
 */
	if (! istty || euid == 0)
	  return 1;
	if (stp->st_uid == euid)
	  return stp->st_mode & S_IWUSR;
	if (stp->st_gid == egid)
	  return stp->st_mode & S_IWGRP;
	return stp->st_mode & S_IWOTH;
}

static void usageErr() {
	char *flags1, *flags2;
	
	switch (identity) {
		case CP:
			flags1 = "pifsmrRvx";
			flags2 = "pifsrRvx";
			break;
		case MV:
			flags1 = "ifsmvx";
			flags2 = "ifsvx";
			break;
		case RM:
			fprintf(stderr, "Usage: rm -ifrRvx] file ...\n");
			exit(1);
		case LN:
			flags1 = "ifsSmrRvx";
			flags1 = "ifsSrRvx";
			break;
		case CPDIR:
			flags1 = "ifvx";
			flags2 = nil;
			break;
		case CLONE:
			flags1 = "ifsSvx";
			flags2 = nil;
			break;
	}
	fprintf(stderr, "Usage: %s [-%s] file1 file2\n", prog, flags1);
	if (flags2 != nil) 
	  fprintf(stderr, "       %s [-%s] file ... dir\n", prog, flags2);
	exit(1);
}

static int eatDir(const char *dir, EntryList **dlpp) {
/* Make a linked list of all the names in a directory */
	DIR *dp;
	struct dirent *entry;
	EntryList *lp;

	if ((dp = opendir(dir)) == nil)
	  return 0;

	while ((entry = readdir(dp)) != nil) {
		if (strcmp(entry->d_name, ".") == 0 ||
			strcmp(entry->d_name, "..") == 0)
		  continue;

		lp = *dlpp = allocate(nil, sizeof(*lp));
		lp->name = allocate(nil, strlen(entry->d_name) + 1);
		strcpy(lp->name, entry->d_name);
		dlpp = &lp->next;
	}
	closedir(dp);
	*dlpp = nil;
	return 1;
}

static void chopDList(EntryList **dlpp) {
/* Chop an entry of a name list. */
	EntryList *junk = *dlpp;
	
	*dlpp = junk->next;
	deallocate(junk->name);
	deallocate(junk);
}

static void dropDList(EntryList *dlp) {
/* Get rid of a whole list. */
	while (dlp != nil) {
		chopDList(&dlp);
	}
}

static int checkExistsOrNot(char *path, struct stat *stp, int mustExist) {
	if ((expand ? stat : lstat)(path, stp) < 0) {
		if (mustExist || errno != ENOENT) {
			report(path);
			return 0;
		}
	}
	return 1;
}
#define checkExists(path, stp)	checkExistsOrNot((path), (stp), 1)

static int checkRecurseDir(char *path, EntryList **dlpp) {
	/* Recursively copy/move/remove/link a directory if -r or -R. */
	if (! rflag) {
		errno = EISDIR;
		report(path);
		return 0;
	}
	/* Gather the names in the directory. */
	if (! eatDir(path, dlpp)) {
		report(path);
		return 0;
	}
	return 1;
}

static int checkReplaceFile(char *path, struct stat *stp, EntryList *dlp, 
				int chkForce) {
	if (! isDir(stp)) {
		if (chkForce && ! fflag) {
			errno = ENOTDIR;
			report(path);
			return 0;
		}
		if (iflag) {
			fprintf(stderr, "Replace %s? ", path);
			if (! affirmative()) {
				dropDList(dlp);
				return 0;
			}
		}
		if (unlink(path) < 0) {
			report(path);
			dropDList(dlp);
			return 0;
		}
		stp->st_ino = 0;
	}
	return 1;
}

static int checkMkdir(char *dstPath, struct stat *srcStp, struct stat *dstStp, 
			EntryList *dlp, int chkMerge) {
	if (! exists(dstStp)) {
		if (! pflag && conforming)
		  srcStp->st_mode &= fcMask;
		if (mkdir(dstPath, srcStp->st_mode | S_IRWXU) < 0 ||
				stat(dstPath, dstStp) < 0) {
			report(dstPath);
			dropDList(dlp);
			return 0;
		}
		if (vflag)
		  printf("mkdir %s\n", dstPath);
	} else {
		if (chkMerge && ! mflag) {
			errno = EEXIST;
			report(dstPath);
			dropDList(dlp);
			return 0;
		}
		if (! pflag) {
			/* Keep the existing attributes. */
			srcStp->st_mode = dstStp->st_mode;
			srcStp->st_uid = dstStp->st_uid;
			srcStp->st_gid = dstStp->st_gid;
			srcStp->st_mtime = dstStp->st_mtime;
		}
	}
	return 1;
}

static int checkPathValid(char *srcPath, char *dstPath, struct stat *srcStp, 
			struct stat *dstStp, EntryList *dlp) {
	if (topDstIno == 0) {
		/* Remember the top destination. */
		topDstDev = dstStp->st_dev;
		topDstIno = dstStp->st_ino;
	}

	if (srcStp->st_ino == topDstIno && srcStp->st_dev == topDstDev) {
		/* E.g. cp -r /shallow /shallow/deep. */
		fprintf(stderr, "%s%s %s/ %s/: infinite recursion avoided\n",
			prog, action != MOVE ? " -r" : "", srcPath, dstPath);
		dropDList(dlp);
		return 0;
	}

	if (xflag && topSrcDev != srcStp->st_dev) {
		/* Don't recurse past a mount point. */
		dropDList(dlp);
		return 0;
	}
	return 1;
}

static int checkRmdir(char *srcPath, int prompt) {
	/* The contents of the source directory should have
	 * been (re)moved above. Get rid of the empty dir.
	 */
	if (prompt) {
		fprintf(stderr, "Remove directory %s? ", srcPath);
		if (! affirmative())
		  return 0;
	}
	if (rmdir(srcPath) < 0) {
		if (errno != ENOTEMPTY)
		  report(srcPath);
		return 0;
	}
	if (vflag)
	  printf("rmdir %s\n", srcPath);
	return 1;
}

static int checkInit(char *srcPath, char *dstPath, struct stat *srcStp, 
			struct stat *dstStp, int depth, int chkSrc, int chkDst) {
	/* st_ino == 0 if not stat()'ed yet, or nonexistent. */
	srcStp->st_ino = 0;
	if (dstStp != NULL) 
	  dstStp->st_ino = 0;

	if (chkSrc && ! checkExists(srcPath, srcStp))
	  return 0;

	if (depth == 0) {
		/* First call: Not cross-device yet, first dst not seen yet,
		 * remember top device number.
		 */
		xDev = 0;
		topDstIno = 0;
		topSrcDev = srcStp->st_dev;
	}

	if (chkDst && ! checkExistsOrNot(dstPath, dstStp, 0))
	  return 0;
	return 1;
}

static void setDirAttrs(char *dstPath, struct stat *srcStp, struct stat *dstStp) {
	/* Set the attributes of a new directory. */
	struct utimbuf ut;

	/* Copy the ownership. */
	if ((pflag || ! conforming) && 
			(dstStp->st_uid != srcStp->st_uid || 
			 dstStp->st_gid != srcStp->st_gid)) {
		if (chown(dstPath, srcStp->st_uid, srcStp->st_gid) < 0) {
			if (errno != EPERM) {
				report(dstPath);
				return;
			}
		}
	}

	/* Copy the mode. */
	if (dstStp->st_mode != srcStp->st_mode) {
		if (chmod(dstPath, srcStp->st_mode) < 0) {
			report(dstPath);
			return;
		}
	}

	/* Copy the file modification time. */
	if (dstStp->st_mtime != srcStp->st_mtime) {
		ut.actime = (action == MOVE) ? srcStp->st_atime : time(nil);
		ut.modtime = srcStp->st_mtime;
		if (utime(dstPath, &ut) < 0) {
			if (errno != EPERM) {
				report(dstPath);
				return;
			}
			fprintf(stderr,
				"%s: Can't set the time of %s\n", prog, dstPath);
			return;
		}
	}
}

static int checkMoveToItself(char *path, struct stat *srcStp, 
			struct stat *dstStp) {
	if (srcStp->st_ino == dstStp->st_ino) {
		fprintf(stderr, 
			"%s: Can't move %s onto itself\n", prog, path);
		exitCode = 1;
		return 0;
	}
	return 1;
}

static int checkConfirm(const char *path, struct stat *stp, char *s) {
	if (iflag || (! fflag && ! writable(stp))) {
		fprintf(stderr, "%s %s? (mode = %03o) ",
			s, path, stp->st_mode & 07777);
		if (! affirmative())
		  return 0;
	}
	return 1;
}
#define confirmReplace(path, stp)	checkConfirm((path), (stp), "Replace")
#define confirmRemove(path, stp)	checkConfirm((path), (stp), "Remove")
#define confirmOverwrite(path, stp)	checkConfirm((path), (stp), "Overwrite")

static int checkRename(char *srcPath, char *dstPath) {
	if (rename(srcPath, dstPath) == 0) {
		/* Success. */
		if (vflag) 
		  printf("mv %s %s\n", srcPath, dstPath);
		return 0;
	}
	if (errno == EXDEV) {
		xDev = 1;
	} else {
		report2(srcPath, dstPath);
		return 0;
	}
	return 1;
}

static int checkRemoveContents(char *path) {
	if (iflag) {
		fprintf(stderr, "Remove contents of %s? ", path);
		if (! affirmative())
		  return 0;
	}
	return 1;
}

static char *linkIsLink(struct stat *stp, const char *file) {
	/* Tell if a file, which stat(2) information in '*stp', has been seen
	 * earlier by this function under a different name. If not return a 
	 * null pointer with errno set to ENOENT, otherwise return the name of
	 * the link. Return a null pointer with an error code in errno for any
	 * error, using E2BIG for a too long file name.
	 *
	 * Use linkIsLink(nil, nil) to reset all bookkeeping.
	 *
	 * Call for a file twice to delete it from the store.
	 */

	typedef struct Link {	/* In-memory link store. */
		struct Link *next;	/* Hash chain on inode number. */
		ino_t ino;			/* File's inode number. */
		off_t off;			/* Offset to more info in temp file. */
	} Link;

	typedef struct DiskLink {	/* On-disk link store. */
		dev_t dev;				/* Device number. */
		char file[PATH_MAX];	/* Name of earlier seen link. */
	} DiskLink;

	static Link *links[256];	/* Hash list of known links. */
	static int tfd = -1;		/* Temp file for file name storage. */
	static DiskLink diskLink;
	Link *lp, **plp;
	size_t len;
	off_t off;

	if (file == nil) {
		/* Reset everything. */
		for (plp = links; plp < arrayLimit(links); ++plp) {
			while ((lp = *plp) != nil) {
				*plp = lp->next;
				free(lp);
			}
		}
		if (tfd != -1)
		  close(tfd);
		tfd = -1;
		return nil;
	}

	/* The file must be a non-directory with more than one link. */
	if (isDir(stp) || stp->st_nlink <= 1) {
		errno = ENOENT;
		return nil;
	}

	plp = &links[stp->st_ino % arraySize(links)];

	while ((lp = *plp) != nil) {
		if (lp->ino == stp->st_ino) {
			/* May have seen this link before. Get it and check. */
			if (lseek(tfd, lp->off, SEEK_SET) == -1)
			  return nil;
			if (read(tfd, &diskLink, sizeof(diskLink)) < 0)
			  return nil;

			/* Only need to check the device number. */
			if (diskLink.dev == stp->st_dev) {
				if (strcmp(file, diskLink.file) == 0) {
					/* Called twice. Forget about this link. */
					*plp = lp->next;
					free(lp);
					errno = ENOENT;
					return nil;
				}

				/* Return the name of the earlier link. */
				return diskLink.file;
			}
		}
		plp = &lp->next;
	}

	/* First time I see this link. Add it to the store. */
	if (tfd == -1) {
		for (;;) {
			char *tmp;

			tmp = tmpnam(nil);
			tfd = open(tmp, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
			if (tfd < 0) {
				if (errno != EEXIST)
				  return nil;
			} else {
				unlink(tmp);
				break;
			}
		}
	}
	if ((len = strlen(file) >= PATH_MAX)) {
		errno = E2BIG;
		return nil;
	}

	diskLink.dev = stp->st_dev;
	strcpy(diskLink.file, file);
	len += offsetof(DiskLink, file) + 1;
	if ((off = lseek(tfd, 0, SEEK_END)) == -1)
	  return nil;
	if (write(tfd, &diskLink, len) != len)
	  return nil;

	if ((lp = malloc(sizeof(*lp))) == nil)
	  return nil;
	lp->next = nil;
	lp->ino = stp->st_ino;
	lp->off = off;
	*plp = lp;
	errno = ENOENT;
	return nil;
}

static int tryLink(const char *srcPath, const char *dstPath, struct stat *srcStp, 
					struct stat *dstStp) {
	char *oldDst;
	int linked;

	if (action == COPY && expand)
	  return 0;

	if ((oldDst = linkIsLink(srcStp, dstPath)) == nil) 
	  return 0;

	/* Try to link the file copied earlier to the new file. */
	if (dstStp->st_ino != 0)
	  unlink(dstPath);

	linked = (link(oldDst, dstPath) == 0);
	if (linked && vflag)
	  printf("ln %s ..\n", oldDst);

	return linked;
}

static int copyModTime(const char *dstPath, struct stat *srcStp, struct stat *dstStp, 
						int condition) {
	if ((pflag || ! conforming) && condition) {
		struct utimbuf ut;

		ut.actime = (action == MOVE ? srcStp->st_atime : time(nil));
		ut.modtime = srcStp->st_mtime;
		if (utime(dstPath, &ut) < 0) {
			if (errno != EPERM) {
				report(dstPath);
				return 0;
			}
			if (pflag) 
			  fprintf(stderr, "%s: Can't set the time of %s\n",
						  prog, dstPath);
		}
	}
	return 1;
}

static int copyFile(const char *srcPath, const char *dstPath, struct stat *srcStp,
			struct stat *dstStp) {
/* Copy one file to another and copy (some of) the attributes. */
	char buf[CHUNK];
	int srcFd, dstFd;
	ssize_t n;

	if (! exists(dstStp)) {
		/* The file doesn't exist yet. */
		if (! S_ISREG(srcStp->st_mode)) {
			/* Making a new mode 666 regular file. */
			srcStp->st_mode = (S_IFREG | 0666) & fcMask;
		} else if (! pflag && conforming) {
			/* Making a new file copying mode with umask applied. */
			srcStp->st_mode &= fcMask;
		}
	} else {
		/* File exists, ask if ok to overwrite if '-i'. */
		if (iflag || (action == MOVE && ! fflag && ! writable(dstStp))) {
			if (! confirmOverwrite(dstPath, dstStp))
			  return 0;
		}

		if (action == MOVE) {
			/* Don't overwrite, remove first. */
			if (unlink(dstPath) < 0 && errno != ENOENT) {
				report(dstPath);
				return 0;
			}
		} else {
			/* Overwrite. */
			if (! pflag) {
				/* Keep the existing mode and ownership. */
				srcStp->st_mode = dstStp->st_mode;
				srcStp->st_uid = dstStp->st_uid;
				srcStp->st_gid = dstStp->st_gid;
			}
		}
	}

	/* Keep the link structure if possible. */
	if (tryLink(srcPath, dstPath, srcStp, dstStp))
	  return 1;

	if ((srcFd = open(srcPath, O_RDONLY)) < 0) {
		report(srcPath);
		return 0;
	}

	dstFd = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC, srcStp->st_mode & 0777);
	if (dstFd < 0 && fflag && errno == EACCES) {
		/* Retry adding a "w" bit. */
		chmod(dstPath, dstStp->st_mode | S_IWUSR);
		dstFd = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC, 0);
	}
	if (dstFd < 0 && fflag && errno == EACCES) {
		/* Retry after trying to delete. */
		unlink(dstPath);
		dstFd = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC, 0);
	}
	if (dstFd < 0) {
		report(dstPath);
		close(srcFd);
		return 0;
	}

	/* Get current parameters. */
	if (fstat(dstFd, dstStp) < 0) {
		report(dstPath);
		close(srcFd);
		close(dstFd);
		return 0;
	}

	/* Copy the little bytes themselves. */
	while ((n = read(srcFd, buf, sizeof(buf))) > 0) {
		char *bp = buf;
		ssize_t r;

		while (n > 0 && (r = write(dstFd, bp, n)) > 0) {
			bp += r;
			n -= r;
		}
		if (r <= 0) {
			if (r == 0) {
				fprintf(stderr, "%s: Warning: EOF writing to %s\n",
							prog, dstPath);
				break;
			}
			fatal(dstPath);
		}
	}

	if (n < 0) {
		report(srcPath);
		close(srcFd);
		close(dstFd);
		return 0;
	}

	close(srcFd);
	close(dstFd);

	/* Copy the ownership. */
	if ((pflag || ! conforming) &&
			S_ISREG(dstStp->st_mode) &&
			(dstStp->st_uid != srcStp->st_uid ||
				dstStp->st_gid != srcStp->st_gid)) {
		if (chmod(dstPath, 0) == 0)
		  dstStp->st_mode &= ~07777;
		if (chown(dstPath, srcStp->st_uid, srcStp->st_gid) < 0) {
			if (errno != EPERM) {
				report(dstPath);
				return 0;
			}
		} else {
			dstStp->st_uid = srcStp->st_uid;
			dstStp->st_gid = srcStp->st_gid;
		}
	}

	if (conforming && 
			S_ISREG(dstStp->st_mode) &&
			(dstStp->st_uid != srcStp->st_uid || 
				dstStp->st_gid != srcStp->st_gid)) {
		/* Suid bits must be cleared in the holy name of 
		 * security (and the assumed user stupidity).
		 */
		srcStp->st_mode &= ~(S_ISUID | S_ISGID);
	}

	/* Copy the mode. */
	if (S_ISREG(dstStp->st_mode) && dstStp->st_mode != srcStp->st_mode) {
		if (chmod(dstPath, srcStp->st_mode) < 0) {
			if (errno != EPERM) {
				report(dstPath);
				return 0;
			}
			fprintf(stderr, "%s: Can't change the mode of %s\n",
						prog, dstPath);
		}
	}

	/* Copy the file modification time. */
	if (! copyModTime(dstPath, srcStp, dstStp, S_ISREG(dstStp->st_mode)))
	  return 0;

	if (vflag) 
	  printf("%s %s ..\n", (action == COPY ? "cp" : "mv"), srcPath);
	return 1;
}

static void doCopy(const char *srcPath, const char *dstPath, 
			struct stat *srcStp, struct stat *dstStp) {
/* Inspect the source file and then copy it. Treatment of symlinks and
 * special files is a bit complicated. The filetype and link-structure are
 * ignored if (expand && !rflag), symlinks and link-structure are ignored
 * if (expand && rflag), everything is copied precisely if !expand.
 */
	int r, linked;

	if (srcStp->st_ino == dstStp->st_ino && 
				srcStp->st_dev == dstStp->st_dev) {
		fprintf(stderr, "%s: can't copy %s onto itself\n", prog, srcPath);
		exitCode = 1;
		return;
	}

	/* You can forget it if the destination is a directory. */
	if (dstStp->st_ino != 0 && S_ISDIR(dstStp->st_mode)) {
		errno = EISDIR;
		report(dstPath);
		return;
	}

	if (S_ISREG(srcStp->st_mode) || (expand && ! rflag)) {
		if (! copyFile(srcPath, dstPath, srcStp, dstStp))
		  return;

		if (action == MOVE && unlink(srcPath) < 0) {
			report(srcPath);
			return;
		}
		return;
	}

	if (dstStp->st_ino != 0) {
		if (iflag || (action == MOVE && ! fflag && ! writable(dstStp))) {
			if (! confirmReplace(dstPath, dstStp))
			  return;
		}
		if (unlink(dstPath) < 0) {
			report(dstPath);
			return;
		}
		dstStp->st_ino = 0;
	}

	/* Apply the file creation mask if so required. */
	if (! pflag && conforming)
	  srcStp->st_mode &= fcMask;

	linked = 0;

	if (S_ISLNK(srcStp->st_mode)) {
		char buf[1024 + 1];

		if ((r = readlink(srcPath, buf, sizeof(buf) - 1)) < 0) {
			report(srcPath);
			return;
		}
		buf[r] = 0;
		r = symlink(buf, dstPath);
		if (vflag && r == 0)
		  printf("ln -s %s %s\n", buf, dstPath);
	} else if (tryLink(srcPath, dstPath, srcStp, dstStp)) {
		linked = 1;
		r = 0;
	} else if (S_ISFIFO(srcStp->st_mode)) {
		//TODO r = mkfifo(dstPath, srcStp->st_mode);
		printf("==== TODO mkfifo\n");
		r = -1;
		if (vflag && r == 0)
		  printf("mkfifo %s\n", dstPath);
	} else if (S_ISBLK(srcStp->st_mode) || S_ISCHR(srcStp->st_mode)) {
		r = mknod(dstPath, srcStp->st_mode, srcStp->st_rdev);
		if (vflag && r == 0) {
			printf("mknod %s %c %d %d\n", 
				dstPath, 
				S_ISBLK(srcStp->st_mode) ? 'b' : 'c',
				MAJOR_DEV(srcStp->st_rdev),
				MINOR_DEV(srcStp->st_rdev));
		}
	} else {
		fprintf(stderr, "%s: %s: odd fileType %5o (not copied)\n",
				prog, srcPath, srcStp->st_mode);
		exitCode = 1;
		return;
	}

	if (r < 0 || lstat(dstPath, dstStp) < 0) {
		report(dstPath);
		return;
	}

	if (action == MOVE && unlink(srcPath) < 0) {
		report(srcPath);
		unlink(dstPath);	/* Don't want it twice. */
		return;
	}

	if (linked)
	  return;

	if (S_ISLNK(srcStp->st_mode))
	  return;

	/* Copy the ownership. */
	if ((pflag || ! conforming) &&
			(dstStp->st_uid != srcStp->st_uid ||
			 dstStp->st_gid != srcStp->st_gid)) {
		if (chown(dstPath, srcStp->st_uid, srcStp->st_gid) < 0) {
			if (errno != EPERM) {
				report(dstPath);
				return;
			}
		}
	}

	/* Copy the file modification time. */
	if (! copyModTime(dstPath, srcStp, dstStp, 1))
	  return;
}

static void doRemove(const char *path, struct stat *stp) {
	if (confirmRemove(path, stp)) {
		if (unlink(path) < 0) 
		  report(path);
		else if (vflag) 
		  printf("rm %s\n", path);
	}
}

static void doLink(const char *srcPath, const char *dstPath, 
			struct stat *srcStp, struct stat *dstStp) {
	PathName sym;
	const char *p;

	if (exists(dstStp) && (iflag || fflag)) {
		if (srcStp->st_ino == dstStp->st_ino) {
			if (fflag)
			  return;
			fprintf(stderr, "%s: Can't link %s onto itself\n",
						prog, srcPath);
			exitCode = 1;
			return;
		}
		if (iflag) {
			fprintf(stderr, "Remove %s?", dstPath);
			if (! affirmative())
			  return;
		}
		errno = EISDIR;
		if (S_ISDIR(dstStp->st_mode) || unlink(dstPath) < 0) {
			report(dstPath);
			return;
		}
	}

	if (! sflag && 
			! (rflag && S_ISLNK(srcStp->st_mode)) && 
			! (Sflag && xDev)) {
		/* A normal link. */
		if (link(srcPath, dstPath) < 0) {
			if (! Sflag || errno != EXDEV) {
				report2(srcPath, dstPath);
				return;
			}
			/* Can't do a cross-device link, we have to symlink. */
			xDev = 1;
		} else {
			if (vflag)
			  printf("ln %s..\n", srcPath);
			return;
		}
	}

	/* Do a symlink. */
	if (! rflag && ! Sflag) {
		/* We can get away with a "don't care if it works" symlink. */
		if (symlink(srcPath, dstPath) < 0) {
			report(dstPath);
			return;
		}
		if (vflag)
		  printf("ln -s %s %s\n", srcPath, dstPath);
		return;
	}

	/* If the source is a symlink then it is simply copied. */
	if (S_ISLNK(srcStp->st_mode)) {
		int r;
		char buf[1024 + 1];

		if ((r = readlink(srcPath, buf, sizeof(buf) - 1)) < 0) {
			report(srcPath);
			return;
		}
		buf[r] = 0;
		if (symlink(buf, dstPath) < 0) {
			report(dstPath);
			return;
		}
		if (vflag)
		  printf("ln -s %s %s\n", buf, dstPath);
		return;
	}

	/* Make a symlink that has to work, i.e. we must be able to access the
	 * source now, and the link must work. 
	 */
	if (dstPath[0] == '/' && srcPath[0] != '/') {
		fprintf(stderr,
			"%s: Symlinking %s to %s is too difficult for me to figure out\n",
			prog, srcPath, dstPath);
		exit(1);
	}

	/* Count the number of subdirectories in the destination file and
	 * add one '..' for each.
	 */
	pathInit(&sym);
	if (srcPath[0] != '/') {
		p = dstPath;
		while (*p != 0) {
			if (p[0] == '.') {
				if (p[1] == '/' || p[1] == 0) {
					/* A "." component; skip. */
					do {
						++p;
					} while (*p == '/');
					continue;
				} else if (p[1] == '.' && (p[2] == '/' || p[2] == 0)) {
					/* A ".." component; oops. */
					switch (PATH_LENGTH(&sym)) {
						case 0:
							fprintf(stderr,
								"%s: Symlinking %s to %s is too difficult for me to figure out\n",
								prog, srcPath, dstPath);
							exit(1);
						case 2:
							pathTrunc(&sym, 0);
							break;
						default:
							pathTrunc(&sym, PATH_LENGTH(&sym) - 3);
					}
					++p;
					do {
						++p;
					} while (*p == '/');
					continue;
				}
			}
			while (*p != 0 && *p != '/') {
				++p;
			}
			while (*p == '/') {
				++p;
			}
			if (*p == 0)
			  break;
			pathAdd(&sym, "..");
		}
	}
	pathAdd(&sym, srcPath);
	
	if (symlink(PATH_NAME(&sym), dstPath) < 0) 
	  report(dstPath);
	else if (vflag)
	  printf("ln -s %s %s\n", PATH_NAME(&sym), dstPath);
	PATH_DROP(&sym);
}

static void recurseDir(PathName *src, PathName *dst, EntryList **dlpp, 
			int depth) {
	size_t slashSrc, slashDst;

	slashSrc = PATH_LENGTH(src);
	slashDst = PATH_LENGTH(dst);

	while (*dlpp != nil) {
		pathAdd(src, (*dlpp)->name);
		if (action != REMOVE)
		  pathAdd(dst, (*dlpp)->name);
		
		do1(src, dst, depth + 1);

		pathTrunc(src, slashSrc);
		pathTrunc(dst, slashDst);
		chopDList(dlpp);
	}
}

static void copyAction(PathName *src, PathName *dst, struct stat *srcStp, 
					struct stat *dstStp, int depth) {
	EntryList *dlp;
	char *srcPath = PATH_NAME(src);
	char *dstPath = PATH_NAME(dst);

	if (! checkInit(srcPath, dstPath, srcStp, dstStp, depth, 1, 1))
	  return;
	if (! isDir(srcStp)) {	
		doCopy(srcPath, dstPath, srcStp, dstStp);
	} else {	
		if (! checkRecurseDir(srcPath, &dlp) ||
			! checkReplaceFile(dstPath, dstStp, dlp, 1) ||
			! checkMkdir(dstPath, srcStp, dstStp, dlp, 0) ||
			! checkPathValid(srcPath, dstPath, srcStp, dstStp, dlp))
		  return;

		recurseDir(src, dst, &dlp, depth);
		setDirAttrs(dstPath, srcStp, dstStp);
	}
}

static void moveAction(PathName *src, PathName *dst, struct stat *srcStp, 
					struct stat *dstStp, int depth) {
	EntryList *dlp;
	char *srcPath = PATH_NAME(src);
	char *dstPath = PATH_NAME(dst);

	if (! checkInit(srcPath, dstPath, srcStp, dstStp, depth, 1, 1))
	  return;
	if (! xDev) {
		if (dstStp->st_ino != 0 && srcStp->st_dev != dstStp->st_dev) {
			/* It's a cross-device rename, i.e. copy and remove. */
			xDev = 1;
		} else if (! mflag || ! exists(dstStp) || ! isDir(dstStp)) {
			/* Try to simply rename the file (not merging trees). */
			if (! checkMoveToItself(srcPath, srcStp, dstStp))
			  return;
			if (exists(dstStp)) {
				if (! confirmReplace(dstPath, dstStp))
				  return;
				if (! isDir(dstStp))
				  unlink(dstPath);
				if (! checkRename(srcPath, dstPath))
				  return;
			}
		}
	} 
	if (! isDir(srcStp)) {
		doCopy(srcPath, dstPath, srcStp, dstStp);
	} else {
		if (! checkRecurseDir(srcPath, &dlp) ||
			! checkReplaceFile(dstPath, dstStp, dlp, 0) ||
			! checkMkdir(dstPath, srcStp, dstStp, dlp, 1) ||
			! checkPathValid(srcPath, dstPath, srcStp, dstStp, dlp)) 
		  return;

		recurseDir(src, dst, &dlp, depth);
		if (! checkRmdir(srcPath, 0))
		  return;
		setDirAttrs(dstPath, srcStp, dstStp);
	}
}

static void removeAction(PathName *src, struct stat *srcStp, int depth) {
	EntryList *dlp;
	char *srcPath = PATH_NAME(src);

	if (! checkInit(srcPath, NULL, srcStp, NULL, depth, 1, 0))
	  return;
	if (! isDir(srcStp)) {
		doRemove(srcPath, srcStp);
	} else {
		if (! checkRecurseDir(srcPath, &dlp))
		  return;
		/* Don't recurse past a mount point. */
		if (xflag && topSrcDev != srcStp->st_dev) 
		  return;
		if (! checkRemoveContents(srcPath))
		  return;
		recurseDir(src, NULL, &dlp, depth);
		if (! checkRmdir(srcPath, iflag))
		  return;
	}
}

static void linkAction(PathName *src, PathName *dst, struct stat *srcStp, 
					struct stat *dstStp, int depth) {
	EntryList *dlp;
	char *srcPath = PATH_NAME(src);
	char *dstPath = PATH_NAME(dst);
	int chkSrc = ! sflag || rflag;

	if (! checkInit(srcPath, dstPath, srcStp, dstStp, depth, chkSrc, 1))
	  return;
	if (srcStp->st_ino == 0 || ! isDir(srcStp)) {
		doLink(srcPath, dstPath, srcStp, dstStp);
	} else {
		if (! checkRecurseDir(srcPath, &dlp) ||
			! checkReplaceFile(dstPath, dstStp, dlp, 1) ||
			! checkMkdir(dstPath, srcStp, dstStp, dlp, 0) ||
			! checkPathValid(srcPath, dstPath, srcStp, dstStp, dlp))
		  return;

		recurseDir(src, dst, &dlp, depth);
		setDirAttrs(dstPath, srcStp, dstStp);
	}
}


static void do1(PathName *src, PathName *dst, int depth) {
/* Perform the appropriate action ona source and destination file. */
	struct stat srcSt, dstSt;

	switch (action) {
		case COPY:
			copyAction(src, dst, &srcSt, &dstSt, depth);
			break;
		case MOVE:
			moveAction(src, dst, &srcSt, &dstSt, depth);
			break;
		case REMOVE:
			removeAction(src, &srcSt, depth);
			break;
		case LINK:
			linkAction(src, dst, &srcSt, &dstSt, depth);
			break;
	}
}

void main(int argc, char **argv) {
	int i;
	char *flags;
	struct stat st;
	PathName src, dst;
	size_t slash;
	char *tmpArgv[argc + 1];
	
	prog = baseName(argv[0]);
	for (i = 0; i < argc; ++i) {
		tmpArgv[i] = argv[i];
	}
	argv = tmpArgv;

	/* Required action. */
	if (strcmp(prog, "cp") == 0) {
		identity = CP;
		action = COPY;
		flags = "pifsmrRvx";
		expand = 1;
	} else if (strcmp(prog, "mv") == 0) {
		identity = MV;
		action = MOVE;
		flags = "ifsmvx";
		rflag = pflag = 1;
	} else if (strcmp(prog, "rm") == 0) {
		identity = RM;
		action = REMOVE;
		flags = "ifrRvx";
	} else if (strcmp(prog, "ln") == 0) {
		identity = LN;
		action = LINK;
		flags = "ifsSmrRvx";
	} else if (strcmp(prog, "cpdir") == 0) {
		identity = CPDIR;
		action = COPY;
		flags = "pifsmrRvx";
		rflag = mflag = pflag = 1;
		conforming = 0;
	} else if (strcmp(prog, "clone") == 0) {
		identity = CLONE;
		action = LINK;
		flags = "ifsSmrRvx";
		rflag = mflag = fflag = 1;
	} else {
		fprintf(stderr,
			"%s: Identity crisis, not called cp, mv, rm, ln, cpdir, or clone\n",
			prog);
		exit(1);
	}

	/* Who am I?, where am I?, how protective am I? */
	euid = geteuid();
	egid = getegid();
	istty = isatty(0);
	fcMask = ~umask(0);

	/* Gather flags. */
	i = 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt = argv[i++] + 1;

		if (opt[0] == '-' && opt[1] == 0)	/* -- */
		  break;

		while (*opt != 0) {
			/* Flag supported? */
			if (strchr(flags, *opt) == nil)
			  usageErr();

			switch (*opt++) {
				case 'p':
					pflag = 1;
					break;
				case 'i':
					iflag = 1;
					if (action == MOVE)
					  fflag = 0;
					break;
				case 'f':
					fflag = 1;
					if (action == MOVE)
					  iflag = 0;
					break;
				case 's':
					if (action == LINK)
					  sflag = 1;
					else	/* Forget about POSIX, do it right. */
					  conforming = 0;	
					break;
				case 'S':
					Sflag = 1;
					break;
				case 'm':
					mflag = 1;
					break;
				case 'r':
					expand = 0;
					/* Fall through */
				case 'R':
					rflag = 1;
					break;
				case 'v':
					vflag = 1;
					break;
				case 'x':
					xflag = 1;
					break;
			}
		}
	}

	switch (action) {
		case REMOVE:
			if (i == argc)
			  usageErr();
			break;
		case LINK:
			/* 'ln dir/file' is to be read as 'ln dir/file .'. */
			if (argc - i == 1)
			  argv[argc++] = ".";
			/* Fall through */
		default:
			if (argc - i < 2)
			  usageErr();
	}

	pathInit(&src);
	pathInit(&dst);

	if (action != REMOVE && 
			! mflag && 
			stat(argv[argc - 1], &st) >= 0 &&
			isDir(&st)) {
		/* The last argument is a directory, this means we have to
		 * throw the whole lot into this directory. This is the
		 * Right Thing unless you use -r.
		 */
		pathAdd(&dst, argv[argc - 1]);
		slash = PATH_LENGTH(&dst);
		do {
			pathAdd(&src, argv[i]);
			pathAdd(&dst, baseName(argv[i]));
			do1(&src, &dst, 0);
			pathTrunc(&src, 0);
			pathTrunc(&dst, slash);
		} while (++i < argc - 1);

	} else if (action == REMOVE) {
		do {
			pathAdd(&src, argv[i]);
			do1(&src, &dst, 0);
			pathTrunc(&src, 0);
		} while (++i < argc);

	} else if (argc - i == 2) {
		pathAdd(&src, argv[i]);
		pathAdd(&dst, argv[i + 1]);
		do1(&src, &dst, 0);
	} else {
		usageErr();
	}
	PATH_DROP(&src);
	PATH_DROP(&dst);
	exit(exitCode);
}








