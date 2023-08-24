#include "util.h"

static void test21a() {
/* Test rename(). */
	int fd, fd2;
	char buf[PATH_MAX + 1], buf1[PATH_MAX + 1], buf2[PATH_MAX + 1];
	struct stat stat1, stat2;

	subTest = 1;

	unlink("A1");
	unlink("A2");
	unlink("A3");
	unlink("A4");
	unlink("A5");
	unlink("A6");
	unlink("A7");

	/* Basic test. Rename A1 to A2 and then A2 to A3. */
	if ((fd = creat("A1", 0666)) < 0) e(1);
	if (write(fd, buf, 20) != 20) e(2);
	if (close(fd) < 0) e(3);
	if (rename("A1", "A2") < 0) e(4);
	if ((fd = open("A2", O_RDONLY)) < 0) e(5);
	if (rename("A2", "A3") < 0) e(6);
	if ((fd2 = open("A3", O_RDONLY)) < 0) e(7);
	if (close(fd) != 0) e(8);
	if (close(fd2) != 0) e(9);
	if (unlink("A3") != 0) e(10);

	/* Now get the absolute path name of the current directory using getcwd()
	 * and use it to test RENAME using different combinations of relative and
	 * absolute path names.
	 */
	if (getcwd(buf, PATH_MAX) == NULL) e(11);
	if ((fd = creat("A4", 0666)) < 0) e(12);
	if (write(fd, buf, 30) != 30) e(13);
	if (close(fd) != 0) e(14);
	strcpy(buf1, buf);
	strcat(buf1, "/A4");
	if (rename(buf1, "A5") != 0) e(15);
	if (access("A5", R_OK | W_OK) != 0) e(16);
	strcpy(buf2, buf);
	strcat(buf2, "/A6");
	if (rename("A5", buf2) != 0) e(17);
	if (access("A6", R_OK | W_OK) != 0) e(18);
	if (access(buf2, R_OK | W_OK) != 0) e(19);
	strcpy(buf1, buf);
	strcat(buf1, "/A6");
	strcpy(buf2, buf);
	strcat(buf2, "/A7");
	if (rename(buf1, buf2) != 0) e(20);
	if (access("A7", R_OK | W_OK) != 0) e(21);
	if (access(buf2, R_OK | W_OK) != 0) e(22);

	/* Try renaming using names like "./A8" */
	if (rename("A7", "./A8") != 0) e(23);
	if (access("A8", R_OK | W_OK) != 0) e(24);
	if (rename("./A8", "A9") != 0) e(25);
	if (access("A9", R_OK | W_OK) != 0) e(26);
	if (rename("./A9", "A10") != 0) e(27);
	if (access("A10", R_OK | W_OK) != 0) e(28);
	if (access("./A10", R_OK | W_OK) != 0) e(29);
	if (unlink("A10") != 0) e(30);

	/* Now see if directories can be renamed. */
	if (system("rm -rf ?uzzy scsi") != 0) e(31);
	if (system("mkdir fuzzy") != 0) e(32);
	if (rename("fuzzy", "wuzzy") != 0) e(33);
	if ((fd = creat("wuzzy/was_a_bear", 0666)) < 0) e(34);
	if (access("wuzzy/was_a_bear", R_OK | W_OK) != 0) e(35);
	if (unlink("wuzzy/was_a_bear") != 0) e(36);
	if (close(fd) != 0) e(37);
	if (rename("wuzzy", "buzzy") != 0) e(38);
	if (system("rmdir buzzy") != 0) e(39);
	
	/* Now start testing the case that 'new' exists. */
	if ((fd = creat("horse", 0666)) < 0) e(40);
	if ((fd2 = creat("sheep", 0666)) < 0) e(41);
	if (write(fd, buf, PATH_MAX) != PATH_MAX) e(42);
	if (write(fd2, buf, 23) != 23) e(43);
	if (stat("horse", &stat1) != 0) e(44);
	if (rename("horse", "sheep") != 0) e(45);
	if (stat("sheep", &stat2) != 0) e(46);
	if (stat1.st_dev != stat2.st_dev) e(47);
	if (stat1.st_ino != stat2.st_ino) e(48);
	if (stat2.st_size != PATH_MAX) e(49);
	if (access("horse", R_OK | W_OK) == 0) e(50);
	if (close(fd) != 0) e(51);
	if (close(fd2) != 0) e(52);
	if (rename("sheep", "sheep") != 0) e(53);
	if (unlink("sheep") != 0) e(54);

	/* Now try renaming something to a directory that already exists. */
	if (system("mkdir fuzzy wuzzy") != 0) e(55);
	if ((fd = creat("fuzzy/was_a_bear", 0666)) < 0) e(56);
	if (close(fd) != 0) e(57);
	if (rename("fuzzy", "wuzzy") != 0) e(58);
	if (system("mkdir scsi") != 0) e(59);
	if (rename("scsi", "wuzzy") == 0) e(60);
	if (errno != EEXIST && errno != ENOTEMPTY) e(61);

	/* Test 14 character names--always tricky. */
	if (rename("wuzzy/was_a_bear", "wuzzy/was_not_a_bear") != 0) e(62);
	if (access("wuzzy/was_not_a_bear", R_OK | W_OK) != 0) e(63);
	if (rename("wuzzy/was_not_a_bear", "wuzzy/was_not_a_duck") != 0) e(64);
	if (access("wuzzy/was_not_a_duck", R_OK | W_OK) != 0) e(65);
	if (rename("wuzzy/was_not_a_duck", "wuzzy/was_a_bird") != 0) e(65);
	if (access("wuzzy/was_a_bird", R_OK | W_OK) != 0) e(66);

	/* Test moves between directories. */
	if (rename("wuzzy/was_a_bird", "beast") != 0) e(67);
	if (access("beast", R_OK | W_OK) != 0) e(68);
	if (rename("beast", "wuzzy/was_a_cat") != 0) e(69);
	if (access("wuzzy/was_a_cat", R_OK | W_OK) != 0) e(70);

	/* Test error conditions. 'scsi' and 'wuzzy/was_a_cat' exist now. */
	if (rename("wuzzy/was_a_cat", "wuzzy/was_a_dog") != 0) e(71);
	if (access("wuzzy/was_a_dog", R_OK | W_OK) != 0) e(72);
	if (chmod("wuzzy", 0) != 0) e(73);

	errno = 0;
	if (rename("wuzzy/was_a_dog", "wuzzy/was_a_pig") != -1) e(74);
	if (errno != EACCES) e(75);

	errno = 0;
	if (rename("wuzzy/was_a_dog", "doggie") != -1) e(76);
	if (errno != EACCES) e(77);

	errno = 0;
	if ((fd = creat("beast", 0666)) < 0) e(78);
	if (close(fd) != 0) e(79);
	if (rename("beast", "wuzzy/was_a_twit") != -1) e(80);
	if (errno != EACCES) e(81);

	errno = 0;
	if (rename("beast", "wuzzy") != -1) e(82);
	if (errno != EISDIR) e(83);

	errno = 0;
	if (rename("beest", "baste") != -1) e(84);
	if (errno != ENOENT) e(85);

	errno = 0;
	if (rename("wuzzy", "beast") != -1) e(86);
	if (errno != ENOTDIR) e(87);

	/* Test prefix rule. */
	errno = 0;
	if (chmod("wuzzy", 0777) != 0) e(88);
	if (unlink("wuzzy/was_a_dog") != 0) e(89);
	strcpy(buf1, buf);
	strcat(buf1, "/wuzzy");
	if (rename(buf, buf1) != -1) e(90);
	if (errno != EINVAL) e(91);

	if (system("rm -rf wuzzy beast scsi") != 0) e(92);
}

static void test21b() {
/* Test mkdir() and rmdir(). */
	int i;
	char name[3];
	struct stat st;

	subTest = 2;

	/* Simple stuff. */
	if (mkdir("D1", 0700) != 0) e(1);
	if (stat("D1", &st) != 0) e(2);
	if (! S_ISDIR(st.st_mode)) e(3);
	if ((st.st_mode & 0777) != 0700) e(4);
	if (rmdir("D1") != 0) e(5);

	/* Make and remove 40 directories. By doing so, the directory has to
	 * grow to 2 blocks. That presents plenty of opportunity for bugs.
	 */
	name[0] = 'D';
	name[2] = '\0';
	for (i = 0; i < 200; ++i) {
		name[1] = 'A' + i;
		if (mkdir(name, 0700 + i % 7) != 0) e(10 + i);
	}
	for (i = 0; i < 200; ++i) {
		name[1] = 'A' + i;
		if (rmdir(name) != 0) e(50 + i);
	}
}

static void test21c() {
/* Test mkdir() and rmdir(). */

	subTest = 3;

	if (mkdir("D1", 0777) != 0) e(1);
	if (mkdir("D1/D2", 0777) != 0) e(2);
	if (mkdir("D1/D2/D3", 0777) != 0) e(3);
	if (mkdir("D1/D2/D3/D4", 0777) != 0) e(4);
	if (mkdir("D1/D2/D3/D4/D5", 0777) != 0) e(5);
	if (mkdir("D1/D2/D3/D4/D5/D6", 0777) != 0) e(6);
	if (mkdir("D1/D2/D3/D4/D5/D6/D7", 0777) != 0) e(7);
	if (mkdir("D1/D2/D3/D4/D5/D6/D7/D8", 0777) != 0) e(8);
	if (mkdir("D1/D2/D3/D4/D5/D6/D7/D8/D9", 0777) != 0) e(9);
	if (access("D1/D2/D3/D4/D5/D6/D7/D8/D9", R_OK | W_OK | X_OK) != 0) e(10);
	if (rmdir("D1/D2/D3/D4/D5/D6/D7/D8/D9") != 0) e(11);
	if (rmdir("D1/D2/D3/D4/D5/D6/D7/D8") != 0) e(12);
	if (rmdir("D1/D2/D3/D4/D5/D6/D7") != 0) e(13);
	if (rmdir("D1/D2/D3/D4/D5/D6") != 0) e(14);
	if (rmdir("D1/D2/D3/D4/D5") != 0) e(15);
	if (rmdir("D1/D2/D3/D4") != 0) e(16);
	if (rmdir("D1/D2/D3") != 0) e(17);
	if (rmdir("D1/D2") != 0) e(18);
	if (rmdir("D1") != 0) e(19);
}

static void test21d() {
/* Test making directories with files and directories in them. */
	int fd1, fd2, fd3, fd4, fd5, fd6, fd7, fd8, fd9;

	subTest = 4;

	if (mkdir("D1", 0777) != 0) e(1);
	if (mkdir("D1/D2", 0777) != 0) e(2);
	if (mkdir("./D1/D3", 0777) != 0) e(3);
	if (mkdir("././D1/D4", 0777) != 0) e(4);
	if ((fd1 = creat("D1/D2/x", 0700)) < 0) e(5);
	if ((fd2 = creat("D1/D2/y", 0700)) < 0) e(6);
	if ((fd3 = creat("D1/D2/z", 0700)) < 0) e(7);
	if ((fd4 = creat("D1/D3/x", 0700)) < 0) e(8);
	if ((fd5 = creat("D1/D3/y", 0700)) < 0) e(9);
	if ((fd6 = creat("D1/D3/z", 0700)) < 0) e(10);
	if ((fd7 = creat("D1/D4/x", 0700)) < 0) e(11);
	if ((fd8 = creat("D1/D4/y", 0700)) < 0) e(12);
	if ((fd9 = creat("D1/D4/z", 0700)) < 0) e(13);
	if (unlink("D1/D2/z") != 0) e(14);
	if (unlink("D1/D2/y") != 0) e(15);
	if (unlink("D1/D2/x") != 0) e(16);
	if (unlink("D1/D3/z") != 0) e(17);
	if (unlink("D1/D3/y") != 0) e(18);
	if (unlink("D1/D3/x") != 0) e(19);
	if (unlink("D1/D4/z") != 0) e(20);
	if (unlink("D1/D4/y") != 0) e(21);
	if (unlink("D1/D4/x") != 0) e(22);
	if (rmdir("D1/D2") != 0) e(23);
	if (rmdir("D1/D3") != 0) e(24);
	if (rmdir("D1/D4") != 0) e(25);
	if (rmdir("D1") != 0) e(26);
	if (close(fd1) != 0) e(27);
	if (close(fd2) != 0) e(28);
	if (close(fd3) != 0) e(29);
	if (close(fd4) != 0) e(30);
	if (close(fd5) != 0) e(31);
	if (close(fd6) != 0) e(32);
	if (close(fd7) != 0) e(33);
	if (close(fd8) != 0) e(34);
	if (close(fd9) != 0) e(35);
}

static void test21e() {
/* Test error conditions. */
	subTest = 5;

	if (mkdir("D1", 0677) != 0) e(1);
	errno = 0;
	if (mkdir("D1/D2", 0777) != -1) e(2);
	if (errno != EACCES) e(3);
	if (chmod("D1", 0577) != 0) e(4);
	errno = 0;
	if (mkdir("D1/D2", 0777) != -1) e(5);
	if (errno != EACCES) e(6);
	if (chmod("D1", 0777) != 0) e(7);
	errno = 0;
	if (mkdir("D1", 0777) != -1) e(8);
	if (errno != EEXIST) e(9);
	errno = 0;
	if (mkdir("D1/D2/x", 0777) != -1) e(14);
	if (errno != ENOENT) e(15);

	/* A particularly nasty test is when the parent has mode 0. Although
	 * this is unlikely to work, it had better not muck up the file system
	 */
	if (mkdir("D1/D2", 0777) != 0) e(16);
	if (chmod("D1", 0) != 0) e(17);
	errno = 0;
	if (rmdir("D1/D2") != -1) e(18);
	if (errno != EACCES) e(19);
	if (chmod("D1", 0777) != 0) e(20);
	if (rmdir("D1/D2") != 0) e(21);
	if (rmdir("D1") != 0) e(22);
}

static int getLink(char *name) {
	struct stat st;

	if (stat(name, &st) != 0) e(1000);
	return st.st_nlink;
}

static void test21f() {
	int fd, D1_before, D1_after, xLink, yLink;

	/* Test case 1: renaming a file within the same directory. */
	subTest = 6;

	if (mkdir("D1", 0777) != 0) e(1);
	if ((fd = creat("D1/x", 0777)) < 0) e(2);
	if (close(fd) != 0) e(3);
	D1_before = getLink("D1");
	xLink = getLink("D1/x");
	if (rename("D1/x", "D1/y") != 0) e(4);
	yLink = getLink("D1/y");
	D1_after = getLink("D1");
	if (D1_before != 2) e(5);
	if (D1_after != 2) e(6);
	if (xLink != 1) e(7);
	if (yLink != 1) e(8);
	if (access("D1/y", R_OK | W_OK | X_OK) != 0) e(9);
	if (unlink("D1/y") != 0) e(10);
	if (rmdir("D1") != 0) e(11);
}

static void test21g() {
	int fd, D1_before, D1_after, D2_before, D2_after, xLink, yLink;

	/* Test case 2: move a file to a new directory. */
	subTest = 7;
	if (mkdir("D1", 0777) != 0) e(1);
	if (mkdir("D2", 0777) != 0) e(2);
	if ((fd = creat("D1/x", 0777)) < 0) e(3);
	if (close(fd) != 0) e(4);
	D1_before = getLink("D1");
	D2_before = getLink("D2");
	xLink = getLink("D1/x");
	if (rename("D1/x", "D2/y") != 0) e(5);
	yLink = getLink("D2/y");
	D1_after = getLink("D1");
	D2_after = getLink("D2");
	if (D1_before != 2) e(6);
	if (D2_before != 2) e(7);
	if (D1_after != 2) e(8);
	if (D2_after != 2) e(9);
	if (xLink != 1) e(10);
	if (yLink != 1) e(11);
	if (access("D2/y", R_OK | W_OK | X_OK) != 0) e(12);
	if (unlink("D2/y") != 0) e(13);
	if (rmdir("D1") != 0) e(14);
	if (rmdir("D2") != 0) e(15);
}

static void test21h() {
	int D1_before, D1_after, xLink, yLink;

	/* Test case 3: renaming a directory within the same directory. */
	subTest = 8;
	if (mkdir("D1", 0777) != 0) e(1);
	if (mkdir("D1/X", 0777) != 0) e(2);
	D1_before = getLink("D1");
	xLink = getLink("D1/X");
	if (rename("D1/X", "D1/Y") != 0) e(3);
	yLink = getLink("D1/Y");
	D1_after = getLink("D1");
	if (D1_before != 3) e(4);
	if (D1_after != 3) e(5);
	if (xLink != 2) e(6);
	if (yLink != 2) e(7);
	if (access("D1/Y", R_OK | W_OK | X_OK) != 0) e(8);
	if (rmdir("D1/Y") != 0) e(9);
	if (getLink("D1") != 2) e(10);
	if (rmdir("D1") != 0) e(11);
}

static void test21i() {
	int D1_before, D1_after, D2_before, D2_after, xLink, yLink;

	/* Test case 4: move a directory to a new directory. */
	subTest = 9;
	if (mkdir("D1", 0777) != 0) e(1);
	if (mkdir("D2", 0777) != 0) e(2);
	if (mkdir("D1/X", 0777) != 0) e(3);
	D1_before = getLink("D1");
	D2_before = getLink("D2");
	xLink = getLink("D1/X");
	if (rename("D1/X", "D2/Y") != 0) e(4);
	yLink = getLink("D2/Y");
	D1_after = getLink("D1");
	D2_after = getLink("D2");
	if (D1_before != 3) e(5);
	if (D2_before != 2) e(6);
	if (D1_after != 2) e(7);
	if (D2_after != 3) e(8);
	if (xLink != 2) e(9);
	if (yLink != 2) e(10);
	if (access("D2/Y", R_OK | W_OK | X_OK) != 0) e(11);
	if (rename("D2/Y", "D1/Z") != 0) e(12);
	if (getLink("D1") != 3) e(13);
	if (getLink("D2") != 2) e(14);
	if (rmdir("D1/Z") != 0) e(15);
	if (getLink("D1") != 2) e(16);
	if (rmdir("D1") != 0) e(17);
	if (rmdir("D2") != 0) e(18);
}

static void test21j() {
/* Now test the same 4 cases, except when the target exists. */

	int fd, D1_before, D1_after, xLink, yLink;

	/* Test case 5: renaming a file within the same directory. */
	subTest = 10;

	if (mkdir("D1", 0777) != 0) e(1);
	if ((fd = creat("D1/x", 0777)) < 0) e(2);
	if (close(fd) != 0) e(3);
	if ((fd = creat("D1/y", 0777)) < 0) e(4);
	if (close(fd) != 0) e(5);
	D1_before = getLink("D1");
	xLink = getLink("D1/x");
	if (rename("D1/x", "D1/y") != 0) e(6);
	yLink = getLink("D1/y");
	D1_after = getLink("D1");
	if (D1_before != 2) e(7);
	if (D1_after != 2) e(8);
	if (xLink != 1) e(9);
	if (yLink != 1) e(10);
	if (access("D1/y", R_OK | W_OK | X_OK) != 0) e(11);
	if (unlink("D1/y") != 0) e(12);
	if (rmdir("D1") != 0) e(13);
	errno = 0;
	if (open("D1/x", O_RDONLY) != -1) e(14);
	if (errno != ENOENT) e(15);
}

static void test21k() {
	int fd, D1_before, D1_after, D2_before, D2_after, xLink, yLink;

	/* Test case 6: move a file to a new directory. */
	subTest = 11;
	if (mkdir("D1", 0777) != 0) e(1);
	if (mkdir("D2", 0777) != 0) e(2);
	if ((fd = creat("D1/x", 0777)) < 0) e(3);
	if (close(fd) != 0) e(4);
	if ((fd = creat("D2/y", 0777)) < 0) e(1003);
	if (close(fd) != 0) e(1004);
	D1_before = getLink("D1");
	D2_before = getLink("D2");
	xLink = getLink("D1/x");
	if (rename("D1/x", "D2/y") != 0) e(5);
	yLink = getLink("D2/y");
	D1_after = getLink("D1");
	D2_after = getLink("D2");
	if (D1_before != 2) e(6);
	if (D2_before != 2) e(7);
	if (D1_after != 2) e(8);
	if (D2_after != 2) e(9);
	if (xLink != 1) e(10);
	if (yLink != 1) e(11);
	if (access("D2/y", R_OK | W_OK | X_OK) != 0) e(12);
	if (unlink("D2/y") != 0) e(13);
	if (rmdir("D1") != 0) e(14);
	if (rmdir("D2") != 0) e(15);
}

static void test21l() {
	int D1_before, D1_after, xLink, yLink;

	/* Test case 7: renaming a directory within the same directory. */
	subTest = 12;
	if (mkdir("D1", 0777) != 0) e(1);
	if (mkdir("D1/X", 0777) != 0) e(2);
	if (mkdir("D1/Y", 0777) != 0) e(1002);
	D1_before = getLink("D1");
	xLink = getLink("D1/X");
	if (rename("D1/X", "D1/Y") != 0) e(3);
	yLink = getLink("D1/Y");
	D1_after = getLink("D1");
	if (D1_before != 4) e(4);
	if (D1_after != 3) e(5);
	if (xLink != 2) e(6);
	if (yLink != 2) e(7);
	if (access("D1/Y", R_OK | W_OK | X_OK) != 0) e(8);
	if (rmdir("D1/Y") != 0) e(9);
	if (getLink("D1") != 2) e(10);
	if (rmdir("D1") != 0) e(11);
}

static void test21m() {
	int D1_before, D1_after, D2_before, D2_after, xLink, yLink;

	/* Test case 8: move a directory to a new directory. */
	subTest = 13;
	if (mkdir("D1", 0777) != 0) e(1);
	if (mkdir("D2", 0777) != 0) e(2);
	if (mkdir("D1/X", 0777) != 0) e(3);
	if (mkdir("D2/Y", 0777) != 0) e(3);
	D1_before = getLink("D1");
	D2_before = getLink("D2");
	xLink = getLink("D1/X");
	if (rename("D1/X", "D2/Y") != 0) e(4);
	yLink = getLink("D2/Y");
	D1_after = getLink("D1");
	D2_after = getLink("D2");
	if (D1_before != 3) e(5);
	if (D2_before != 3) e(6);
	if (D1_after != 2) e(7);
	if (D2_after != 3) e(8);
	if (xLink != 2) e(9);
	if (yLink != 2) e(10);
	if (access("D2/Y", R_OK | W_OK | X_OK) != 0) e(11);
	if (rename("D2/Y", "D1/Z") != 0) e(12);
	if (getLink("D1") != 3) e(13);
	if (getLink("D2") != 2) e(14);
	if (rmdir("D1/Z") != 0) e(15);
	if (getLink("D1") != 2) e(16);
	if (rmdir("D1") != 0) e(17);
	if (rmdir("D2") != 0) e(18);
}

static void test21n() {
/* Test trying to remove . and .. */
	subTest = 14;
	if (mkdir("D1", 0777) != 0) e(1);
	if (chdir("D1") != 0) e(2);
	if (rmdir(".") == 0) e(3);
	if (rmdir("..") == 0) e(4);
	if (mkdir("D2", 0777) != 0) e(5);
	if (mkdir("D3", 0777) != 0) e(6);
	if (mkdir("D4", 0777) != 0) e(7);
	if (rmdir("D2/../D3/../D4") != 0) e(8);
	if (rmdir("D2/../D3/../D2/..") == 0) e(9);
	if (rmdir("D2/../D3/../D2/../..") == 0) e(10);
	errno = 0;
	if (rename("D2", "D2/D4") == 0) e(1000);
	if (errno != EINVAL) e(1001);
	if (rmdir("../D1/../D1/D3") != 0) e(11);
	if (rmdir("./D2/../D2") != 0) e(12);
	if (chdir("..") != 0) e(13);
	if (rmdir("D1") != 0) e(14);
}

void main(int argc, char **argv) {
	int i;

	if (geteuid() == 0 || getuid() == 0) {
		printf("Test 21 cannot run as root; test aborted\n");
		exit(1);
	}
	setup(argc, argv);

	for (i = 0; i < 1; ++i) {
		if (m & 0001) test21a();
		if (m & 0002) test21b();
		if (m & 0004) test21c();
		if (m & 0010) test21d();
		if (m & 0020) test21e();
		if (m & 0040) test21f();
		if (m & 0100) test21g();
		if (m & 0200) test21h();
		if (m & 0400) test21i();
		if (m & 01000) test21j();
		if (m & 02000) test21k();
		if (m & 04000) test21l();
		if (m & 010000) test21m();
		if (m & 020000) test21n();
	}
	quit();
}




