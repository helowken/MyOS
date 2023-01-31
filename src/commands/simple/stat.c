/* usage: stat [-] [-all] -<field> [-<field> ...] [file1, file2 file3 ...]
 *
 * where <field> is one of the struct stat fields without the leading "st_".
 *	   The three times can be printed out as human times by requesting
 *	   -Ctime instead of -ctime (upper case 1st letter).
 *	   - means take the file names from stdin.
 *	   -0.. means fd0..
 *	   no files means all fds.
 * 
 * output: if only on field is specified, that fields' contents are printed.
 *		   if more than one field is specified, the output is
 *	   file field1: f1val, field2: f2val, etc
 */

#include "sys/types.h"
#include "errno.h"
#include "limits.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "sys/stat.h"
#include "minix/minlib.h"

#define addr(x)		((void *) &sb.x)
#define size(x)		sizeof(sb.x)
#define equal(s, t)	(strcmp(s, t) == 0)
#define S_IREAD		S_IRUSR
#define S_IWRITE	S_IWUSR
#define S_IEXEC		S_IXUSR

static char *prog;
static struct stat sb;
static int firstFile = 1;

typedef struct {
	char *f_name;		/* Field name in stat */
	u_char *f_addr;		/* Address of the field in 'st' */
	u_short f_size;		/* Size of the object, needed for pointer */
	u_short f_print;	/* Show this field? */
} Field;

static Field fields[] = {
	{ "dev",     addr(st_dev),       size(st_dev),        0 },
	{ "ino",     addr(st_ino),       size(st_ino),        0 },
	{ "mode",    addr(st_mode),      size(st_mode),       0 },
	{ "nlink",   addr(st_nlink),     size(st_nlink),      0 },
	{ "uid",     addr(st_uid),       size(st_uid),        0 },
	{ "gid",     addr(st_gid),		 size(st_gid),		  0 },
	{ "rdev",	 addr(st_rdev),		 size(st_rdev),		  0 },
	{ "size",    addr(st_size),		 size(st_size),       0 },
	{ "Atime",   addr(st_atime),     size(st_atime),      0 },
	{ "atime",   addr(st_atime),     size(st_atime),      0 },
	{ "Mtime",   addr(st_mtime),     size(st_mtime),      0 },
	{ "mtime",   addr(st_mtime),     size(st_mtime),      0 },
	{ "Ctime",   addr(st_ctime),     size(st_ctime),      0 },
	{ "ctime",   addr(st_ctime),     size(st_ctime),      0 },
	{ NULL }
};

static void usageErr() {
	usage(prog, "[-] [-fd] [-all] [-s] [-field ...] [file1 ...]\n");
}

static void rwx(mode_t mode, char *bit) {
	if (mode & S_IREAD)
	  bit[0] = 'r';
	if (mode & S_IWRITE)
	  bit[1] = 'w';
	if (mode & S_IEXEC)
	  bit[2] = 'x';
}

static void printIt(struct stat *st, Field *f, int n) {
	if (n > 1)
	  printf("%s: ", f->f_name);
	if (equal(f->f_name, "mode")) {
		char bit[11];

		printf("%07lo, ", (u_long) st->st_mode);
		strcpy(bit, "----------");

		switch (st->st_mode & S_IFMT) {
			case S_IFDIR:	
				bit[0] = 'd';
				break;
			case S_IFIFO:
				bit[0] = 'p';
				break;
			case S_IFCHR:
				bit[0] = 'c';
				break;
			case S_IFBLK:
				bit[0] = 'b';
				break;
			case S_IFLNK:
				bit[0] = 'l';
				break;
		}
		rwx(st->st_mode, bit + 1);
		rwx(st->st_mode << 3, bit + 4);
		rwx(st->st_mode << 6, bit + 7);
		if (st->st_mode & S_ISUID)
		  bit[3] = 's';
		if (st->st_mode & S_ISGID)
		  bit[6] = 's';
		if (st->st_mode & S_ISVTX)
		  bit[9] = 't';
		printf("\"%s\"", bit);
	} else if (equal("Ctime", f->f_name)) {
		printf("%.24s (%lu)", ctime(&st->st_ctime), (u_long) st->st_ctime);
		f[1].f_print = 0;
	} else if (equal("Mtime", f->f_name)) {
		printf("%.24s (%lu)", ctime(&st->st_mtime), (u_long) st->st_mtime);
		f[1].f_print = 0;
	} else if (equal("Atime", f->f_name)) {
		printf("%.24s (%lu)", ctime(&st->st_atime), (u_long) st->st_atime);
		f[1].f_print = 0;
	} else if (equal("ctime", f->f_name)) {
		printf("%lu", (u_long) st->st_ctime);
	} else if (equal("mtime", f->f_name)) {
		printf("%lu", (u_long) st->st_mtime);
	} else if (equal("atime", f->f_name)) {
		printf("%lu", (u_long) st->st_atime);
	} else if (equal("rdev", f->f_name)) {
		printf("%d (0x%x)", st->st_rdev, st->st_rdev);
	} else {
		switch (f->f_size) {
			case sizeof(char):
				printf("%d", * (u_char *) f->f_addr);
				break;
			case sizeof(short):
				printf("%u", (u_int) * (u_short *) f->f_addr);
				break;
			case sizeof(int):
				printf("%u", * (u_int *) f->f_addr);
				break;
#if LONG_MAX != INT_MAX 
			case sizeof(long):
				printf("%lu", * (u_long *) f->f_addr);
				break;
#endif
			default:
				fprintf(stderr, "\nProgram error: bad '%s' field size %d\n",
							f->f_name, f->f_size);
				break;
		}
	}
}

static void printStat(struct stat *st, int nprint) {
	int j;
	int firstField = 1;
	for (j = 0; fields[j].f_name; ++j) {
		if (fields[j].f_print) {
			if (! firstField)
			  fputc('\n', stdout);
			printIt(st, &fields[j], nprint);
			firstField = 0;
		}
	}
	fputc('\n', stdout);
	firstFile = 0;
}

void main(int argc, char **argv) {
	int i, j, nprint = 0, files = 0;
	char buf[PATH_MAX], *check;
	int sym = 0, ret, fromStdin = 0;
	int err;
	u_long fd;

	if ((prog = strrchr(argv[0], '/')) == NULL)
	  prog = argv[0];
	else
	  ++prog;
	if (equal(prog, "lstat"))
	  sym = 1;

	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			if (equal(argv[i], "-")) {
				++fromStdin;
				++files;
				continue;
			} 
			if (equal("-all", argv[i])) {
				for (j = 0; fields[j].f_name; ++j) {
					++nprint;
					++fields[j].f_print;
				}
				continue;
			}
			if (equal("-s", argv[i])) {
				sym = 1;
				continue;
			}
			fd = strtoul(argv[i] + 1, &check, 0);
			if (check != argv[i] + 1 && *check == '\0') {
				++files;
				continue;
			}
			for (j = 0; fields[j].f_name; ++j) {
				if (equal(fields[j].f_name, &argv[i][1])) {
					++nprint;
					++fields[j].f_print;
					break;
				}
			}
			if (! fields[j].f_name) {
				if (! equal("-?", argv[i]))
				  fprintf(stderr, "stat: %s: bad field\n", argv[i]);
				usageErr();
			}
		} else {
			++files;
		}
	}
	if (! nprint) {
		for (j = 0; fields[j].f_name; ++j) {
			++nprint;
			++fields[j].f_print;
		}
	}

	if (fromStdin)
	  ++files;	/* We don't know how many files come from stdin. */

	if (files == 0) {	/* Stat all file descriptors. */
		for (i = 0; i < OPEN_MAX; ++i) {
			err = fstat(i, &sb);
			if (err == -1 && errno == EBADF)
			  continue;
			if (err == 0) {
				if (! firstFile)
				  fputc('\n', stdout);
				printf("fd %d:\n", i);
				printStat(&sb, nprint);
			} else {
				fprintf(stderr, "%s: fd %d: %s\n", prog, i, strerror(errno));
				ret = 1;
			}
		}
		exit(ret);
	}

	for (i = 1; i < argc; ++i) {
		if (equal(argv[i], "-")) {
			while (fgets(buf, sizeof(buf), stdin)) {
				char *p = strchr(buf, '\n');
				if (p)
				  *p = 0;
				if (! sym)
				  err = stat(buf, &sb);
				if (sym || (err != 0 && errno == ENOENT)) 
				  err = lstat(argv[i], &sb);
				if (err == -1) {
					fprintf(stderr, "%s: %s: %s\n",
						prog, buf, strerror(errno));
					ret = 1;
				} else {
					if (! firstFile)
					  fputc('\n', stdout);
					printf("%s:\n", buf);
					printStat(&sb, nprint);
				}
			}
			continue;
		}
		if (argv[i][0] == '-') {
			fd = strtoul(argv[i] + 1, &check, 10);
			if (check == argv[i] + 1 || *check != '\0')
			  continue;
			if (fd >= INT_MAX) {
				err = -1;
				errno = EBADF;
			} else {
				err = fstat((int) fd, &sb);
			}
			if (err != -1) {
				if (! firstFile)
				  fputc('\n', stdout);
				if (files != 1)
				  printf("fd %lu:\n", fd);
				printStat(&sb, nprint);
			} else {
				fprintf(stderr, "fd %lu: %s\n", fd, strerror(errno));
				ret = 1;
			}
			continue;
		}
		if (! sym)
		  err = stat(argv[i], &sb);
		if (sym || (err != 0 && errno == ENOENT))
		  err = lstat(argv[i], &sb);
		if (err != -1) {
			if (! firstFile)
			  fputc('\n', stdout);
			if (files != 1)
			  printf("%s\n", argv[i]);
			printStat(&sb, nprint);
		} else {
			fprintf(stderr, "%s: %s: %s\n", prog, argv[i], strerror(errno));
			ret = 1;
		}
	}
	exit(ret);
}
