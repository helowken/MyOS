#include "util.h"

#define diff	strcmp

void main(int argc, char **argv) {
	if (diff(argv[0], "t11b")) e(31);
	if (diff(argv[1], "abc")) e(32);
	if (diff(argv[2], "defghi")) e(33);
	if (diff(argv[3], "j")) e(34);
	if (argv[4] != 0) e(35);
	if (argc != 4) e(36);

	exit(75);
}
