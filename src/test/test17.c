#include "util.h"

#define PDP_NOHANG	1
#define MAX_ERR		2

#define USER_ID		12
#define GROUP_ID	1
#define FF			3		/* First free File Descriptions */
#define USER		1		/* uid */
#define GROUP		0		/* gid */


#define ARRAY_SIZE	256		
#define PIPE_SIZE	(4096 * 7)	
#define MAX_OPEN	17		/* Maximum number of extra open files */
#define MAX_LINK	0177	
#define LINK_COUNT	MAX_LINK
#define MASK		0777
#define END_FILE	0


#define OK		0
#define FAIL	-1

#define R		O_RDONLY
#define W		O_WRONLY
#define RW		O_RDWR

#define RWX		7

#define NIL		""
#define UMASK	"umask"
#define CREAT	"creat"
#define WRITE	"write"
#define READ	"read"
#define OPEN	"open"
#define CLOSE	"close"
#define LSEEK	"lseek"
#define ACCESS	"access"
#define CHDIR	"chdir"
#define CHMOD	"chmod"
#define LINK	"link"
#define UNLINK	"unlink"
#define PIPE	"pipe"
#define STAT	"stat"
#define FSTAT	"fstat"
#define DUP		"dup"
#define UTIME	"utime"


char *file[20] = {"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", 
	"f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15", "f16", "f17", "f18", "f19"};
char *fileNames[8] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
char *dir[8] = {"d---", "d--x", "d-w-", "d-wx", "dr--", "dr-x", "drw-", "drwx"};


static void err(int number, char *scall, char *name) {
	++errCnt;
	if (errCnt > MAX_ERR) {
		printf("Too many errors;  test aborted\n");
		quit();
	}
	printf("Error:  \t");
	switch (number) {
		case 0:
			printf("%s: illegal initial value.", scall);
			break;
		case 1:
			printf("%s: %s value returned.", scall, name);
			break;
		case 2:
			printf("%s: accepting illegal %s.", scall, name);
			break;
		case 3:
			printf("%s: accepting non-existing file.", scall);
			break;
		case 4:
			printf("%s: could search non-searchable dir (%s).", scall, name);
			break;
		case 5:
			printf("%s: cannot %s %s.", scall, scall, name);
			break;
		case 7:
			printf("%s: incorrect %s.", scall, name);
			break;
		case 8:
			printf("%s: wrong values.", scall);
			break;
		case 9:
			printf("%s: accepting too many %s files.", scall, name);
			break;
		case 10:
			printf("%s: even a superuser can't do anything!", scall);
			break;
		case 11:
			printf("%s: could %s.", scall, name);
			break;
		case 12:
			printf("%s: could write in non-writable dir (%s).", scall, name);
			break;
		case 13:
			printf("%s: wrong filedes returned (%s).", scall, name);
			break;
		case 100:
			printf("%s: %s.", scall, name);
			break;
		default:
			printf("error number does not exist!\n");
	}
	printf("\n");
}

void tryClose(int fd, char *name) {
	if (close(fd) != OK) 
	  err(5, CLOSE, name);
}

void tryUnlink(char *fileName) {
	if (unlink(fileName) != 0)
	  err(5, UNLINK, fileName);
}

static void Remove(int fd, char *fileName) {
	tryClose(fd, fileName);
	tryUnlink(fileName);
}

void chmod8Dirs(int sw) {
	int mode;
	int i;

	if (sw == 8)
	  mode = 0;
	else
	  mode = sw;

	for (i = 0; i < 8; ++i) {
		chmod(dir[i], 040000 + mode * 0100);
		if (sw == 8)
		  ++mode;
	}
}


void cleanupTheMess() {
	int i;
	char dirName[6];

	/* First remove 'a lot' files */
	for (i = 0; i < MAX_OPEN; ++i) {
		tryUnlink(file[i]);
	}

	/* Unlink the files in dir 'drwx' */
	if (chdir("drwx") != OK)
	  err(5, CHDIR, "to 'drwx'");
	else {
		for (i = 0; i < 8; ++i) {
			tryUnlink(fileNames[i]);
		}
		if (chdir("..") != OK)
		  err(5, CHDIR, "to '..'");
	}

	/* Before unlinking files in some dirs, make them writable */
	chmod8Dirs(RWX);
	
	/* Unlink files in other dirs */
	tryUnlink("d-wx/rwx");
	tryUnlink("dr-x/rwx");
	tryUnlink("drw-/rwx");

	/* Unlink dirs */
	for (i = 0; i < 8; ++i) {
		strcpy(dirName, "d");
		strcat(dirName, fileNames[i]);

		rmdir(dirName);
	}
}

static int getMode(char *name) {
	struct stat st;

	if (stat(name, &st) != OK) {
		err(5, STAT, name);
		return st.st_mode;	/* Return a mode which will cause error in the
							   calling function (file/dir bit) */
	}
	return st.st_mode & 07777;	/* Take last 4 bits */
}

static void test01() {
/* test UAMSK */
	int oldValue, newValue, tempValue;
	int n;

	if ((oldValue = umask(0777)) != 0) err(0, UMASK, NIL);

	/* Special test: only the lower 9 bits (protection bits) may 
	 * participate. ~0777 means: 111 000 000 000. Giving this to umask
	 * must not change any value. 
	 */
	if ((newValue = umask(~0777)) != 0777) err(1, UMASK, "illegal");
	if (oldValue == newValue) err(11, UMASK, "not change mask");

	if ((tempValue = umask(0)) != 0) err(2, UMASK, "values");

	/* Now test all possible modes of umask on a file. */
	for (newValue = MASK; newValue >= 0; newValue -= 0111) {
		tempValue = umask(newValue);
		if (tempValue != oldValue) {
			err(1, UMASK, "illegal");
			break;
		} else if ((n = creat("file01", 0777)) < 0)
		  err(5, CREAT, "'file01'");
		else {
			tryClose(n, "'file01'");
			if (getMode("file01") != (MASK & ~newValue))
			  err(7, UMASK, "mode computed");
			tryUnlink("file01");
		}
		oldValue = newValue;
	}

	/* The loop has terminated with umask(0) */
	if ((tempValue = umask(0)) != 0)
	  err(7, UMASK, "umask may influence rest of tests!");
}

static void putFileInDir(char *dirName, int mode) {
	int n;

	if (chdir(dirName) != OK) 
	  err(5, CHDIR, "to dirName (putFileInDir)");
	else {
		if ((n = creat(fileNames[mode], mode * 0100)) < 0)
		  err(13, CREAT, fileNames[mode]);
		else
		  tryClose(n, fileNames[mode]);

		if (chdir("..") != OK)
		  err(5, CHDIR, "to previous dir (putFileInDir)");
	}
}

static void makeAndFillDirs() {
	int mode, i;

	for (i = 0; i < 8; ++i) {
		if (mkdir(dir[i], 0700) != 0) 
		  err(100, "mkdir", dir[i]);
	}

	for (mode = 0; mode < 8; ++mode) {
		putFileInDir("drwx", mode);
	}

	putFileInDir("d-wx", RWX);
	putFileInDir("dr-x", RWX);
	putFileInDir("drw-", RWX);

	chmod8Dirs(8);
}

static void check(char *scall, int number) {
	if (errno != number) 
	  printf("Error:  \t%s: bad errno-value: %d should have been: %d\n",
				  scall, errno, number);
}

 int openAlot() {
	int i;

	for (i = 0; i < MAX_OPEN; ++i) {
		if (open(file[i], R) == FAIL)
		  break;
	}
	if (i == 0)
	  err(5, "openAlot", "at all");
	return i;
}

static int closeAlot(int number) {
	int i, count = 0;

	if (number > MAX_OPEN)
	  err(5, "closeAlot", "accept this argument");
	else {
		for (i = FF; i < number + FF; ++i) {
			if (close(i) != OK)
			  ++count;
		}
	}
	return number - count;	/* Return number of closed files */
}

static void initArray(char *a) {
	int i;

	i = 0;
	while (i++ < ARRAY_SIZE) {
		*a++ = 'a' + (i % 26);
	}
}

static void clearArray(char *b) {
	int i;

	i = 0;
	while (i++ < ARRAY_SIZE) {
		*b++ = '0';
	}
}

static void test02() {
/* test CREAT */
	int n, n1, mode;
	char a[ARRAY_SIZE], b[ARRAY_SIZE];
	struct stat st;

	mode = 0;
	/* Create twenty files, check fd */
	for (n = 0; n < MAX_OPEN; ++n) {
		if (creat(file[n], mode) != FF + n)
		  err(13, CREAT, file[n]);
		else {
			if (getMode(file[n]) != mode)
			  err(7, CREAT, "mode set while creating many files");

			/* Change mode of file to standard mode, we want to
			 * use a lot (20) of files to be opened later, see
			 * openAlot(), closeAlot().
			 */
			if (chmod(file[n], 0700) != OK)
			  err(5, CHMOD, file[n]);
		}
		mode = (mode + 0100) % 01000;
	}

	/* Already twenty files opened; opening another has to fail */
	if (creat("file02", 0777) != FAIL)
	  err(9, CREAT, "created");
	else
	  check(CREAT, EMFILE);

	/* Close all files: seems blunt, but it isn't because we've
	 * checked all fd's already.
	 */
	if ((n = closeAlot(MAX_OPEN)) < MAX_OPEN)
	  err(5, CLOSE, "MAX_OPEN files");

	/* Creat 1 file twice; check */
	if ((n = creat("file02", 0777)) < 0)
	  err(5, CREAT, "'file02'");
	else {
		initArray(a);
		if (write(n, a, ARRAY_SIZE) != ARRAY_SIZE) 
		  err(1, WRITE, "bad");

		if ((n1 = creat("file02", 0755)) < 0)	/* recreate 'file01' */
		  err(5, CREAT, "'file02' (2nd time)");
		else {
			/* Fd should be at the top after recreation */
			if (lseek(n1, 0L, SEEK_END) != 0)
			  err(11, CREAT, "not truncate file by recreation");
			else {
				/* Try to write on recreated file */
				clearArray(b);

				if (lseek(n1, 0L, SEEK_SET) != 0)
				  err(5, LSEEK, "to top of 2nd fd 'file02'");
				if (write(n1, a, ARRAY_SIZE) != ARRAY_SIZE)
				  err(1, WRITE, "(2) bad");

				/* In order to read we've to close and open again */
				tryClose(n1, "'file02'  (2nd creation)");
				if ((n1 = open("file02", RW)) < 0)
				  err(5, OPEN, "'file02'  (2nd recreation)");

				/* Continue */
				if (lseek(n1, 0L, SEEK_SET) != 0)
				  err(5, LSEEK, "to top 'file02'(2nd fd) (2)");
				if (read(n1, b, ARRAY_SIZE) != ARRAY_SIZE)
				  err(1, READ, "wrong");

				if (memcmp(a, b, ARRAY_SIZE) != OK)
				  err(11, CREAT, "not really truncate file by recreation");
			}
			if (getMode("file02") != 0777)
			  err(11, CREAT, "not maintain mode by recreation");
			tryClose(n1, "recreated 'file02'");
		}
		Remove(n, "file02");
	}

	/* Give 'creat' wrong input: dir not searchable */
	if (creat("drw-/file02", 0777) != FAIL)
	  err(4, CREAT, "'drw-'");
	else
	  check(CREAT, EACCES);

	/* Dir not writable */
	if (creat("dr-x/file02", 0777) != FAIL)
	  err(12, CREAT, "'dr-x/file02'");
	else
	  check(CREAT, EACCES);

	/* Dir not writable */
	if (creat("drwx/r-x", 0777) != FAIL)
	  err(11, CREAT, "recreate non-writable file");
	else
	  check(CREAT, EACCES);

	/* Try to creat a dir */
	if ((n = creat("dir", 040777)) != FAIL) {
		if (fstat(n, &st) != OK)
		  err(5, FSTAT, "'dir'");
		else if (st.st_mode != (mode_t) 0100777)
		  err(11, CREAT, "'creat' a new directory");
		Remove(n, "dir");
	}

	/* File is an existing dir */
	if (creat("drwx", 0777) != FAIL)
	  err(11, CREAT, "create an existing dir!");
	else
	  check(CREAT, EISDIR);
}

static void test08() {
/* Test chdir to searchable dir */
	if (chdir("drwx") != OK)
	  err(5, CHDIR, "to accessible dir");
	else if (chdir("..") != OK)
	  err(11, CHDIR, "not return to '..'");

	/* Check the chdir(".") and chdir("..") mechanism */
	if (chdir("drwx") != OK)
	  err(5, CHDIR, "to 'drwx'");
	else {
		if (chdir(".") != OK)
		  err(5, CHDIR, "to working dir (.)");

		/* If we still are in 'drwx', we should be able to access
		 * file 'rwx'.
		 */
		if (access("rwx", F_OK) != OK)
		  err(5, CHDIR, "rightly to '.'");

		/* Try to return to previous dir ('/' !!) */
		if (chdir("././../././d--x/../d--x/././..") != OK)
		  err(5, CHDIR, "to motherdir (..)");

		/* Check whether we are back in '/' */
		if (chdir("d--x") != OK)
		  err(5, CHDIR, "rightly to a '..'");
	}

	/* Return to '..' */
	if (chdir("..") != OK)
	  err(5, CHDIR, "to '..'");

	if (chdir("././././drwx") != OK)
	  err(11, CHDIR, "not follow a path");
	else if (chdir("././././..") != OK)
	  err(11, CHDIR, "not return to path");

	/* Try giving chdir wrong parameters */
	if (chdir("drwx/rwx") != FAIL)
	  err(11, CHDIR, "chdir to a file");
	else
	  check(CHDIR, ENOTDIR);

	if (chdir("drw-") != FAIL)
	  err(4, CHDIR, "'/drw-'");
	else
	  check(CHDIR, EACCES);
}

static void test09() {
/* test CHMOD */
	int n;
	
	/* Prepare file09 */
	if ((n = creat("drwx/file09", 0644)) != FF)
	  err(5, CREAT, "'drwx/file09'");

	tryClose(n, "'file09'");

	/* Try to chmod a file, check and restore old values, check */
	if (chmod("drwx/file09", 0700) != OK)
	  err(5, CHMOD, "'drwx/file09'");
	else {
		/* Check protection */
		if (getMode("drwx/file09") != 0700)
		  err(7, CHMOD, "mode");

		/* Test if chmod accepts just fileNames too */
		if (chdir("drwx") != OK)
		  err(5, CHDIR, "to '/drwx'");
		else if (chmod("file09", 0177) != OK)
		  err(5, CHMOD, "'h1'");
		else if (getMode("../drwx/file09") != 0177)
		  err(7, CHMOD, "restored mode");
	}

	/* Try setuid and setgid */
	if (chmod("file09", 04777) != OK || getMode("file09") != 04777)
	  err(11, CHMOD, "not set uid-bit");
	if (chmod("file09", 02777) != OK || getMode("file09") != 02777)
	  err(11, CHMOD, "not set gid-bit");

	/* Remove testFile */
	tryUnlink("file09");

	if (chdir("..") != OK)
	  err(5, CHDIR, "to '..'");

	/* Try to chmod directory */
	if (chmod("d---", 0777) != OK)
	  err(5, CHMOD, "dir 'd---'");
	else {
		if (getMode("d---") != 0777)
		  err(7, CHMOD, "protection value");
		if (chmod("d---", 0000) != OK)
		  err(5, CHMOD, "dir 'a' 2nd time");

		/* Check if old value has been restored */
		if (getMode("d---") != 0000)
		  err(7, CHMOD, "restored protection value");
	}

	/* Try to make chmod failures. */
	if (chmod("non-file", 0777) != FAIL)
	  err(3, CHMOD, NIL);
	else
	  check(CHMOD, ENOENT);
}

static void getNew(char name[]) {
/* Every call changes string 'name' to a string alphabetically
 * higher. Start with "aaaaa", next value: "aaaab".
 * N.B. after "aaaaz" comes "aaabz" nad not "aaaba" (not needed).
 * The last possibility will be "zzzzz".
 * Total # possibilities: 26+25*4 = 126 = MAX_LINK - 1 (exactly needed)
 */
	int i;

	for (i = 4; i >= 0; --i) {
		if (name[i] != 'z') {
			++name[i];
			break;
		}
	}
}

static int linkAlot(char *bigBoss) {
	int i;
	static char employee[6] = "aaaaa";

	/* Every file has already got 1 link, so link 0176 times */
	for (i = 1; i < LINK_COUNT; ++i) {
		if (link(bigBoss, employee) != OK) 
		  break;
		else
		  getNew(employee);
	}
	return i - 1;
}

static int unlinkAlot(int number) {
	int j;
	static char employee[6] = "aaaaa";
	
	for (j = 0; j < number; ++j) {
		if (unlink(employee) != OK)
		  break;
		else
		  getNew(employee);
	}
	return j;
}

static void test10() {
/* test LINK/UNLINK */
	int n, n1;
	char a[ARRAY_SIZE], b[ARRAY_SIZE], *f, *lf;

	f = "file10";
	lf = "linkFile10";

	if ((n = creat(f, 0702)) != FF)
	  err(13, CREAT, f);
	else {
		/* Now link correctly */
		if (link(f, lf) != OK)
		  err(5, LINK, lf);
		else if ((n1 = open(lf, RW)) < 0)
		  err(5, OPEN, "'linkFile10'");
		else {
			initArray(a);
			clearArray(b);

			/* Write on 'file10' means being able to read
			 * through linked fd.
			 */
			if (write(n, a, ARRAY_SIZE) != ARRAY_SIZE) 
			  err(1, WRITE, "bad");
			if (read(n1, b, ARRAY_SIZE) != ARRAY_SIZE)
			  err(1, READ, "bad");
			if (memcmp(a, b, ARRAY_SIZE) != OK)
			  err(8, "r/w", NIL);

			/* Clean up: unlink and close (twice): */
			Remove(n, f);
			tryClose(n1, "'linkFile10'");

			/* Check if "linkFile" exists and the info on
			 * it is correct ('file' has been deleted) 
			 */
			if ((n1 = open(lf, R)) < 0)
			  err(5, OPEN, "'linkFile10'");
			else {
				/* See if 'linkFile' still contains 0..511 ? */
				clearArray(b);
				if (read(n1, b, ARRAY_SIZE) != ARRAY_SIZE)
				  err(1, READ, "bad");
				if (memcmp(a, b, ARRAY_SIZE) != OK)
				  err(8, "r/w", NIL);
				
				tryClose(n1, "'linkFile10' 2nd time");
				tryUnlink(lf);
			}
		}
	}

	/* Try if unlink fails with incorrect parameters */
	/* File does not exist: */
	if (unlink("non-file") != FAIL)
	  err(2, UNLINK, "name");
	else
	  check(UNLINK, ENOENT);

	/* Dir can't be written */
	if (unlink("dr-x/rwx") != FAIL)
	  err(11, UNLINK, "could unlink in non-writable dir.");
	else
	  check(UNLINK, EACCES);

	/* Try to unlink a dir being user */
	if (unlink("drwx") != FAIL)
	  err(11, UNLINK, "unlink dir's as user");
	else
	  check(UNLINK, EPERM);

	/* Try giving link wrong input */

	/* First try if link fails with incorrect parameters.
	 * name1 does not exist.
	 */
	if (link("non-file", "linkFile") != FAIL)
	  err(2, LINK, "1st name");
	else
	  check(LINK, ENOENT);

	/* Name2 exists already */
	if (link("drwx/rwx", "drwx/rw-") != FAIL)
	  err(2, LINK, "2nd name");
	else
	  check(LINK, EEXIST);

	/* Directory of name2 not writable: */
	if (link("drwx/rwx", "dr-x/linkFile") != FAIL)
	  err(11, LINK, "link non-writable file");
	else
	  check(LINK, EACCES);

	/* Try to link a dir, being a user */
	 if (link("drwx", "linkFile") != FAIL)
	   err(11, LINK, "link a dir without superuser!");
	 else
	   check(LINK, EPERM);

	 /* File has too many links */
	if ((n = linkAlot("drwx/rwx")) != LINK_COUNT - 1)
	  err(5, LINK, "many files");
	if (unlinkAlot(n) != n)
	  err(5, UNLINK, "all linked files");
}

static void onSigPipe(int sig) {
}

static void test11() {
	int n, fd[2];
	char a[ARRAY_SIZE], b[ARRAY_SIZE];

	if (pipe(fd) != OK)
	  err(13, PIPE, NIL);
	else {
		/* Try reading and writing on a pipe */
		initArray(a);
		clearArray(b);

		if (write(fd[1], a, ARRAY_SIZE) != ARRAY_SIZE)
		  err(5, WRITE, "on pipe");
		else if (read(fd[0], b, ARRAY_SIZE / 2) != ARRAY_SIZE / 2)
		  err(5, READ, "on pipe (2nd time)");
		else if (memcmp(a, b, ARRAY_SIZE / 2) != OK)
		  err(7, PIPE, "values read/written");
		else if (read(fd[0], b, ARRAY_SIZE / 2) != ARRAY_SIZE / 2)
		  err(5, READ, "on pipe 2");
		else if (memcmp(&a[(ARRAY_SIZE / 2)], b, ARRAY_SIZE / 2) != OK)
		  err(7, PIPE, "pipe created");

		/* Try to let the pipe make a mistake. */
		if (write(fd[0], a, ARRAY_SIZE) != FAIL)
		  err(11, WRITE, "write on fd[0]");
		else
		  check(PIPE, EBADF);
		if (read(fd[1], b, ARRAY_SIZE) != FAIL)
		  err(11, READ, "read on fd[1]");
		else 
		  check(PIPE, EBADF);

		tryClose(fd[1], "'fd[1]'");

		/* Now we shouldn't be able to read, because fd[1] has been closed */
		if (read(fd[0], b, ARRAY_SIZE) != END_FILE)
		  err(2, PIPE, "'fd[1]'");

		tryClose(fd[0], "'fd[0]'");
	}
	if (pipe(fd) < 0)
	  err(5, PIPE, "2nd time");
	else {
		/* Test lseek on a pipe: should fail */
		if (write(fd[1], a, ARRAY_SIZE) != ARRAY_SIZE)
		  err(5, WRITE, "on pipe (2nd time)");
		if (lseek(fd[1], 10L, SEEK_SET) != FAIL)
		  err(11, LSEEK, "lseek on a pipe");
		else
		  check(PIPE, ESPIPE);

		/* Eat half of the pipe: no writing should be possible */
		tryClose(fd[0], "'fd[0]'  (2nd time)");

		signal(SIGPIPE, onSigPipe);
		if (write(fd[1], a, ARRAY_SIZE) != FAIL)
		  err(11, WRITE, "write on wrong pipe");
		else
		  check(PIPE, EPIPE);
		signal(SIGPIPE, SIG_DFL);

		tryClose(fd[1], "'fd[1]'  (2nd time)");
	}

	if (pipe(fd) < 0)
	  err(5, PIPE, "3rd time");
	else {
		for (n = 0; n < PIPE_SIZE / ARRAY_SIZE; ++n) {
			if (write(fd[1], a, ARRAY_SIZE) != ARRAY_SIZE)
			  err(5, WRITE, "on pipe (3rd time) 4K ");
		}
		tryClose(fd[1], "'fd[1]' (3rd time)");

		for (n = 0; n < PIPE_SIZE / ARRAY_SIZE; ++n) {
			if (read(fd[0], b, ARRAY_SIZE) < ARRAY_SIZE)
			  err(5, READ, "from pipe (3rd time) 4K");
		}
		tryClose(fd[0], "'fd[0]' (3rd time)");
	}

	/* Test opening a lot of files */
	if ((n = openAlot()) != MAX_OPEN)
	  err(5, OPEN, "MAX_OPEN files");
	if (pipe(fd) != FAIL)
	  err(9, PIPE, "open");
	else
	  check(PIPE, EMFILE);
	if (closeAlot(n) != n)
	  err(5, CLOSE, "all opened files");
}

static void writeStandards(int fd, char a[]) {
	/* Write must return written account of numbers */
	if (write(fd, a, ARRAY_SIZE) != ARRAY_SIZE)
	  err(1, WRITE, "bad");

	/* Try giving 'write' wrong input */
	/* Wrong fd */
	if (write(-1, a, ARRAY_SIZE) != FAIL)
	  err(2, WRITE, "fd");
	else
	  check(WRITE, EBADF);
	
	if (write(fd, a, -ARRAY_SIZE) != FAIL)
	  err(2, WRITE, "length");
	else
	  check(WRITE, EINVAL);
}

static void test03() {
/* test WRITE */
	int n, n1;
	int fd[2];
	char a[ARRAY_SIZE];

	initArray(a);

	/* Test write after a CREAT */
	if ((n = creat("file03", 0700)) != FF)
	  err(13, CREAT, "'file03'");
	else {
		writeStandards(n, a);
		tryClose(n, "'file03'");
	}

	/* Test write after an OPEN */
	if ((n = open("file03", W)) < 0)
	  err(5, OPEN, "'file03'");
	else
	  writeStandards(n, a);

	/* Test write after a DUP */
	if ((n1 = dup(n)) < 0)
	  err(5, DUP, "'file03'");
	else {
		writeStandards(n1, a);
		tryClose(n1, "duplicated fd 'file03'");
	}

	/* Remove testFile */
	Remove(n, "file03");

	/* Test write after a PIPE */
	if (pipe(fd) < 0)
	  err(5, PIPE, NIL);
	else {
		writeStandards(fd[1], a);
		tryClose(fd[0], "'fd[0]'");
		tryClose(fd[1], "'fd[1]'");
	}

	/* Last test: does write check protections ? */
	if ((n = open("drwx/r--", R)) < 0)
	  err(5, OPEN, "'drwx/r--'");
	else {
		if (write(n, a, ARRAY_SIZE) != FAIL)
		  err(11, WRITE, "write on non-write file");
		else 
		  check(WRITE, EBADF);
		tryClose(n, "'drwx/r--'");
	}
}

static void readStandards(int fd, char a[]) {
	char b[ARRAY_SIZE];

	clearArray(b);
	if (read(fd, b, ARRAY_SIZE) != ARRAY_SIZE)
	  err(1, READ, "bad");
	else if (memcmp(a, b, ARRAY_SIZE) != OK)
	  err(7, "read/write", "values");
	else if (read(fd, b, ARRAY_SIZE) != END_FILE)
	  err(11, READ, "read beyond endOfFile");

	/* Try giving read wrong input: wrong fd */
	if (read(-1, b, ARRAY_SIZE) != FAIL)
	  err(2, READ, "fd");
	else
	  check(READ, EBADF);

	/* Wrong length */
	if (read(fd, b, -ARRAY_SIZE) != FAIL)
	  err(2, READ, "length");
	else 
	  check(READ, EINVAL);
}

static void readMore(int fd, char a[]) {
/* Separated from readStandards() because the PIPE test
 * would fail.
 */
	char b[ARRAY_SIZE];

	if (lseek(fd, (long) (ARRAY_SIZE / 2), SEEK_SET) != ARRAY_SIZE / 2)
	  err(5, LSEEK, "to location ARRAY_SIZE / 2");

	clearArray(b);
	if (read(fd, b, ARRAY_SIZE) != ARRAY_SIZE / 2)
	  err(1, READ, "bad");

	if (memcmp(&a[(ARRAY_SIZE / 2)], b, ARRAY_SIZE / 2) != OK)
	  err(7, READ, "from location ARRAY_SIZE / 2");
}

static void test04() {
/* test READ */
	int n, n1, fd[2];
	char a[ARRAY_SIZE];

	/* Test read after creat */
	if ((n = creat("file04", 0700)) != FF)
	  err(13, CREAT, "'file04'");
	else {
		/* Closing and opening needed before writing */
		tryClose(n, "'file04'");
		if ((n = open("file04", RW)) < 0)
		  err(5, OPEN, "'file04'");

		initArray(a);

		if (write(n, a, ARRAY_SIZE) != ARRAY_SIZE)
		  err(1, WRITE, "bad");
		else {
			if (lseek(n, 0L, SEEK_SET) != OK)
			  err(5, LSEEK, "'file04'");
			readStandards(n, a);
			readMore(n, a);
		}
		tryClose(n, "'file04'");
	}

	/* Test read after OPEN */
	if ((n = open("file04", R)) < 0)
	  err(5, OPEN, "'file04'");
	else {
		readStandards(n, a);
		readMore(n, a);
		tryClose(n, "'file04'");
	}

	/* Test read after DUP */
	if ((n = open("file04", R)) < 0)
	  err(5, OPEN, "'file04'");
	if ((n1 = dup(n)) < 0)
	  err(5, DUP, "'file04'");
	else {
		readStandards(n1, a);
		readMore(n1, a);
		tryClose(n1, "duplicated fd 'file04'");
	}

	/* Remove testFile */
	Remove(n, "file04");

	/* Test read after pipe */
	if (pipe(fd) < 0)
	  err(5, PIPE, NIL);
	else {
		if (write(fd[1], a, ARRAY_SIZE) != ARRAY_SIZE) 
		  err(5, WRITE, "'fd[1]'");
		else {
			tryClose(fd[1], "'fd[1]'");
			readStandards(fd[0], a);
		}
		tryClose(fd[0], "'fd[0]'");
	}

	/* Last test: try to read a read-protected file */
	if ((n = open("drwx/-wx", W)) < 0)
	  err(5, OPEN, "'drwx/-wx'");
	else {
		if (read(n, a, ARRAY_SIZE) != FAIL)
		  err(11, READ, "read a non-read. file");
		else
		  check(READ, EBADF);
		tryClose(n, "'/drwx/-wx'");
	}
}

static void tryOpen(char *fileName, int mode, int test) {
	int n;

	if ((n = open(fileName, mode)) != test)
	  err(11, OPEN, "break through file permission with an incorrect mode");
	if (n != FAIL)
	  tryClose(n, fileName);
}

static void test05() {
/* test OPEN/CLOSE */
	int n, n1, mode, fd[2];
	char b[ARRAY_SIZE];

	/* Test open after creat */
	if ((n = creat("file05", 0700)) != FF) 
	  err(13, CREAT, "'file05'");
	else {
		if ((n1 = open("file05", RW)) != FF + 1)
		  err(13, OPEN, "'file05' after creation");
		tryClose(n1, "'file05' (open after creation)");

		tryClose(n, "'file05'");
		if ((n = open("file05", R)) != FF)
		  err(13, OPEN, "after closing");
		else 
		  tryClose(n, "'file05' (open after closing)");

		/* Remove testFile */
		tryUnlink("file05");
	}

	/* Test all possible modes, tryOpen not only opens file (sometimes)
	 * but closes files too (when opended)
	 */
	if ((n = creat("file05", 0700)) < 0)
	  err(5, CREAT, "'file05' (2nd time)");
	else {
		tryClose(n, "file05");
		for (mode = 0; mode <= 0700; mode += 0100) {
			if (chmod("file05", mode) != OK)
			  err(5, CHMOD, "'file05'");

			if (mode <= 0100) {
				tryOpen("file05", R, FAIL);
				tryOpen("file05", W, FAIL);
				tryOpen("file05", RW, FAIL);
			} else if (mode >= 0200 && mode <= 0300) {
				tryOpen("file05", R, FAIL);
				tryOpen("file05", W, FF);
				tryOpen("file05", RW, FAIL);
			} else if (mode >= 0400 && mode <= 0500) {
				tryOpen("file05", R, FF);
				tryOpen("file05", W, FAIL);
				tryOpen("file05", RW, FAIL);
			} else {
				tryOpen("file05", R, FF);
				tryOpen("file05", W, FF);
				tryOpen("file05", RW, FF);
			}
		}
	}

	/* Test opening existing file */
	if ((n = open("drwx/rwx", R)) < 0)
	  err(13, OPEN, "existing file");
	else {
		if ((n1 = dup(n)) < 0)
		  err(13, DUP, "'drwx/rwx'");
		else {
			tryClose(n1, "duplicated fd 'drwx/rwx'");

			if (read(n1, b, ARRAY_SIZE) != FAIL)
			  err(11, READ, "on cloed dupped fd 'drwx/rwx'");
			else 
			  check(READ, EBADF);

			if (read(n, b, ARRAY_SIZE) == FAIL)
			  err(13, READ, "on fd '/drwx/rwx'");
		}
		tryClose(n, "'drwx/rwx'");
	}

	/* Test close after PIPE */
	if (pipe(fd) < 0)
	  err(13, PIPE, NIL);
	else {
		tryClose(fd[1], "'fd[1]'");

		/* Fd[1] really should be closed now; check */
		clearArray(b);
		if (read(fd[0], b, ARRAY_SIZE) != END_FILE)
		  err(11, READ, "read on empty pipe (and fd[1] was closed)");
		tryClose(fd[0], "'fd[0]'");
	}

	/* Try to open a non-existing file */
	if (open("non-file", R) != FAIL)
	  err(11, OPEN, "open non-existing file");
	else 
	  check(OPEN, ENOENT);

	/* Dir does not exist */
	if (open("dzzz/file05", R) != FAIL)
	  err(11, OPEN, "open in an non-existing dir");
	else
	  check(OPEN, ENOENT);

	/* Dir is not searchable */
	if (open("drw-/rwx", R) != FAIL)
	  err(11, OPEN, "open in an non-searchable dir");
	else
	  check(OPEN, EACCES);

	tryUnlink("file05");

	/* File is not readable */
	if (open("drwx/-wx", R) != FAIL)
	  err(11, OPEN, "open unreadable file for reading");
	else
	  check(OPEN, EACCES);

	/* File is not writable */
	if (open("drwx/r-x", W) != FAIL)
	  err(11, OPEN, "open unwritable file for writing");
	else
	  check(OPEN, EACCES);

	/* Try opening more than MAX_OPEN files */
	if ((n = openAlot()) != MAX_OPEN)
	  err(13, OPEN, "MAX_OPEN files");
	else if (open("drwx/rwx", RW) != FAIL)
	  err(9, OPEN, "open");
	else 
	  check(OPEN, EMFILE);
	if (closeAlot(n) != n)
	  err(5, CLOSE, "all opened files");

	/* Can close make mistakes? */
	if (close(-1) != FAIL)
	  err(2, CLOSE, "fd");
	else
	  check(CLOSE, EBADF);
}

static void test06() {
/* test LSEEK */
	char a[ARRAY_SIZE], b[ARRAY_SIZE];
	int fd;

	if ((fd = open("drwx/rwx", RW)) != FF)
	  err(13, OPEN, "'drwx/rwx'");
	else {
		initArray(a);
		if (write(fd, a, 10) != 10)
		  err(1, WRITE, "bad");
		else {
			/* Lseek back to begin file */
			if (lseek(fd, 0L, SEEK_SET) != 0)
			  err(5, LSEEK, "to begin file");
			else if (read(fd, b, 10) != 10)
			  err(1, READ, "bad");
			else if (memcmp(a, b, 10) != OK)
			  err(7, LSEEK, "values r/w after lseek to begin");
			/* Lseek to endOfFile */
			if (lseek(fd, 0L, SEEK_END) != 10)
			  err(5, LSEEK, "to end of file");
			else if (read(fd, b, 1) != END_FILE)
			  err(7, LSEEK, "read at end of file");
			/* Lseek beyond file */
			if (lseek(fd, 10L, SEEK_CUR) != 20)
			  err(5, LSEEK, "beyond end of file");
			else if (write(fd, a, 10) != 10)
			  err(1, WRITE, "bad");
			else {
				/* Lseek to begin second write */
				if (lseek(fd, 20L, SEEK_SET) != 20)
				  err(5, LSEEK, "'/drwx/rwx'");
				if (read(fd, b, 10) != 10)
				  err(1, READ, "bad");
				else if (memcmp(a, b, 10) != OK)
				  err(7, LSEEK, "values read after lseek 20");
			}
		}


		/* Lseek to position before begin of file */
		if (lseek(fd, -1L, SEEK_SET) != FAIL)
		  err(11, LSEEK, "lseek before beginning of file");
		else  
		  check(LSEEK, EINVAL);


		/* Lseek with invalid position */
		if (lseek(fd, 0L, 4) != FAIL)
		  err(11, LSEEK, "lseek invalid position");
		else  
		  check(LSEEK, EINVAL);

		tryClose(fd, "'drwx/rwx'");
	}

	/* Lseek on invalid fd */
	if (lseek(-1, 0L, SEEK_SET) != FAIL)
	  err(2, LSEEK, "fd");
	else
	  check(LSEEK, EBADF);
}

static void tryAccess(char *fileName, int mode, int test) {
	if (access(fileName, mode) != test)
	  err(100, ACCESS, "incorrect access on a file (tryAccess)");
}

static void accessStandards() {
	int i, mode = 0;

	/* '---' */
	for (i = 0; i < 8; ++i) {
		if (i == 0)
		  tryAccess(fileNames[mode], i, OK);
		else
		  tryAccess(fileNames[mode], i, FAIL);
	}
	++mode;	

	/* '--x' */
	for (i = 0; i < 8; ++i) {
		if (i < 2)
		  tryAccess(fileNames[mode], i, OK);
		else
		  tryAccess(fileNames[mode], i, FAIL);
	}
	++mode;

	/* '-w-' */
	for (i = 0; i < 8; ++i) {
		if (i == 0 || i ==2)
		  tryAccess(fileNames[mode], i, OK);
		else
		  tryAccess(fileNames[mode], i, FAIL);
	}
	++mode;

	/* '-wx' */
	for (i = 0; i < 8; ++i) {
		if (i < 4)
		  tryAccess(fileNames[mode], i, OK);
		else
		  tryAccess(fileNames[mode], i, FAIL);
	}
	++mode;

	/* 'r--' */
	for (i = 0; i < 8; ++i) {
		if (i == 0 || i == 4)
		  tryAccess(fileNames[mode], i, OK);
		else
		  tryAccess(fileNames[mode], i, FAIL);
	}
	++mode;

	/* 'r-x' */
	for (i = 0; i < 8; ++i) {
		if (i == 0 || i == 1 || i == 4 || i == 5)
		  tryAccess(fileNames[mode], i, OK);
		else
		  tryAccess(fileNames[mode], i, FAIL);
	}
	++mode;

	/* 'rw-' */
	for (i = 0; i < 8; ++i) {
		if (i % 2 == 0)
		  tryAccess(fileNames[mode], i, OK);
		else
		  tryAccess(fileNames[mode], i, FAIL);
	}
	++mode;

	/* 'rwx' */
	for (i = 0; i < 8; ++i) {
		tryAccess(fileNames[mode], i, OK);
	}
}

static void test07() {
/* test ACCESS */

	/* Check with proper parameters */
	if (access("drwx/rwx", RWX) != OK)
	  err(5, ACCESS, "accessible file");

	if (access("./././drwx/././rwx", F_OK) != OK)
	  err(5, ACCESS, "'./././drwx/././rwx'");

	/* Check 8 files with 8 different modes on 8 accesses */
	if (chdir("drwx") != OK)
	  err(5, CHDIR, "'drwx'");

	accessStandards();

	if (chdir("..") != OK)
	  err(5, CHDIR, "'..'");

	/* Check several wrong combinations */
	/* File does not exist */
	if (access("non-file", F_OK) != FAIL)
	  err(11, ACCESS, "access non-existing file");
	else
	  check(ACCESS, ENOENT);

	/* Non-searchable dir */
	if (access("drw-/rwx", F_OK) != FAIL)
	  err(4, ACCESS, "'drw-'");
	else
	  check(ACCESS, EACCES);

	/* Searchable dir, but wrong file-mode */
	if (access("drwx/--x", RWX) != FAIL)
	  err(11, ACCESS, "a non accessible file");
	else
	  check(ACCESS, EACCES);
}

static void test() {
	umask(0);		
	
	if (m & 00001) test01();
	if (m & 00002) makeAndFillDirs();
	if (m & 00004) test02();
	if (m & 00010) test03();
	if (m & 00020) test04();
	if (m & 00040) test05();
	if (m & 00100) test06();
	if (m & 00200) test07();
	if (m & 00400) test08();
	if (m & 01000) test09();
	if (m & 02000) test10();
	if (m & 04000) test11();
	umask(022);
}

void main(int argc, char **argv) {
	int n;

	if (geteuid() == 0 || getuid() == 0) {
		printf("Test 17 cannot run as root; test aborted\n");
		exit(1);
	}

	setup(argc, argv);

	if (fork()) {
		wait(&n);
		cleanupTheMess();
		quit();
	} else {
		test();
		exit(0);
	}
}
