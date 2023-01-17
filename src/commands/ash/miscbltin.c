



/* Miscelaneous builtins. */

#include "shell.h"
#include "output.h"
#include "error.h"
#include "sys/stat.h"

int readCmd(int argc, char **argv) {
	printf("=== readCmd\n");//TODO
	return 0;
}


int umaskCmd(int argc, char **argv) {
	int mask;
	char *p;
	int i;

	if ((p = argv[1]) == NULL) {
		INTOFF;
		mask = umask(0);
		umask(mask);
		INTON;
		out1Format("%.4o\n", mask);	
	} else {
		mask = 0;
		do {
			if ((unsigned) (i = *p - '0') >= 8)
			  error("Illegal number: %s", argv[1]);
			mask = (mask << 3) + i;
		} while (*++p != '\0');
		umask(mask);
	}
	return 0;
}
