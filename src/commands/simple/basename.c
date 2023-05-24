#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define EOS	'\0'

int main(int argc, char **argv) {
	char *resultString;		/* The pointer into argv[1]. */
	char *temp;				/* Used to move around in argv[1]. */
	int suffixLen;			/* Length of the suffix. */
	int suffixStart;		/* Where the suffix should start. */

	/* Check for the correct number of arguments. */
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: basename string [suffix] \n");
		exit(EXIT_FAILURE);
	}

	/* Check for all /'s */
	for (temp = argv[1]; *temp == '/'; ++temp) {	/* Move to the next char. */
	}
	if (*temp == EOS) {
		printf("/\n");
		exit(EXIT_SUCCESS);
	}

	/* Build the basename. */
	resultString = argv[1];

	/* Find the last /'s */
	temp = strrchr(resultString, '/');

	if (temp != NULL) {
		/* Remove trailing /'s. */
		while (*(temp + 1) == EOS && *temp == '/') {
			*temp-- = EOS;
		}

		/* Set resultString to last part of path. */
		if (*temp != '/')
		  temp = strrchr(resultString, '/');
		if (temp != NULL && *temp == '/')
		  resultString = temp + 1;
	}

	/* Remove the suffix, if any. */
	if (argc > 2) {
		suffixLen = strlen(argv[2]);
		suffixStart = strlen(resultString) - suffixLen;
		if (suffixStart > 0 &&
				strcmp(resultString + suffixStart, argv[2]) == 0)
		  *(resultString + suffixStart) = EOS;
	}

	/* Print the resultant string. */
	printf("%s\n", resultString);
	return 0;
}
