#define DELIMITER	':'

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
	char *path, *cp;
	char buf[400];
	char prog[400];
	char pathBuf[512];
	int quit, none;
	int exitCode = 0;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s cmd [cmd, ..]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	argv[argc] = 0;
	for (++argv; *argv; ++argv) {
		quit = 0;
		none = 1;

		if ((path = getenv("PATH")) == NULL) {
			fprintf(stderr, "Null path.\n");
			exit(EXIT_FAILURE);
		}
		strcpy(pathBuf, path);
		path = pathBuf;
		cp = path;

		while (1) {
			cp = strchr(path, DELIMITER);
			if (cp == NULL)
			  ++quit;
			else
			  *cp = '\0';

			if (strcmp(path, "") == 0 && quit == 0) 
			  sprintf(buf, "./%s", *argv);
			else
			  sprintf(buf, "%s/%s", path, *argv);

			path = ++cp;

			if (access(buf, X_OK) == 0) {
				printf("%s\n", buf);
				none = 0;
			}

			sprintf(prog, "%s.%s", buf, "prg");
			if (access(prog, X_OK) == 0) {
				printf("%s\n", prog);
				none = 0;
			}

			sprintf(prog, "%s.%s", buf, "ttp");
			if (access(prog, X_OK) == 0) {
				printf("%s\n", prog);
				none = 0;
			}

			sprintf(prog, "%s.%s", buf, "tos");
			if (access(prog, X_OK) == 0) {
				printf("%s\n", prog);
				none = 0;
			}

			if (quit) {
				if (none) {
					fprintf(stderr, "No %s in %s\n", *argv, getenv("PATH"));
					exitCode = 1;
				}
				break;
			}
		}
	}
	return exitCode;	
}
