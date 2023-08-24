#include "util.h"

#define diff	strcmp

void main(int argc, char **argv, char **envp) {
/* See if arguments passed ok. */
	char aa[4];

	if (diff(argv[0], "t11a")) e(21);
	if (diff(argv[1], "arg0")) e(22);
	if (diff(argv[2], "arg1")) e(23);
	if (diff(argv[3], "arg2")) e(24);
	if (diff(envp[0], "spring")) e(25);
	if (diff(envp[1], "summer")) e(26);
	if (argc != 4) e(27);

	/* Now see if the files are ok. */
	if (read(3, aa, 4) != 2) e(28);
	if (aa[0] != 7 || aa[1] != 9) e(29);

	if (getuid() == 10) e(30);
	if (geteuid() != 10) e(31);
	if (getgid() == 20) e(32);
	if (getegid() != 20) e(33);

	if (open("t1", 0) < 0) e(34);
	if (open("t2", 0) < 0) e(35);
	exit(100);
}
