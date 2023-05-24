#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <minix/minlib.h>
#include <stdio.h>

#define CHUNK_SIZE	(2048 * sizeof(char *))

static char *prog;
static int unbuffered;
static char inBuf[CHUNK_SIZE];
static char outBuf[CHUNK_SIZE];
static char *outp = outBuf;

static char STDIN[] = "standard input";
static char STDOUT[] = "standard output";

static int exitCode = 0;

static void output(char *buf, size_t count) {
	ssize_t n;

	while (count > 0) {
		n = write(STDOUT_FILENO, buf, count);
		if (n <= 0) {
			if (n < 0) 
			  fatal(prog, STDOUT);
			
			stdErr("cat: ");
			stdErr(STDOUT);
			stdErr(": EOF\n");
			exit(1);
		}
		buf += n;
		count -= n;
	}
}

static void copyOut(char *file, int fd) {
	int n;

	while (1) {
		n = read(fd, inBuf, CHUNK_SIZE);
		if (n < 0)
		  fatal(prog, file);
		if (n == 0)
		  return;
		if (unbuffered || (outp == outBuf && n == CHUNK_SIZE)) {
			output(inBuf, n);
		} else {
			int bytesLeft;

			bytesLeft = &outBuf[CHUNK_SIZE] - outp;
			if (n <= bytesLeft) {
				memcpy(outp, inBuf, (size_t) n);
				outp += n;
			} else {
				memcpy(outp, inBuf, (size_t) bytesLeft);
				output(outBuf, CHUNK_SIZE);
				n -= bytesLeft;
				memcpy(outBuf, inBuf + bytesLeft, (size_t) n);
				outp = outBuf + n;
			}
		}
	}
}

int main(int argc, char **argv) {
	int i, fd;
	struct stat st;

	prog = getProg(argv);
	i = 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt = argv[i] + 1;

		if (opt[0] == 0)	/* - */
		  break;
		++i;
		if (opt[0] == '-' && opt[1] == 0)	/* -- */
		  break;

		while (*opt != 0) {
			switch (*opt++) {
				case 'u':
					unbuffered = 1;
					break;
				default:
					usage(prog, "[-u] [file ...]");
			}
		}
	}

	if (i >= argc) {
		copyOut(STDIN, STDIN_FILENO);
	} else {
		while (i < argc) {
			char *file = argv[i++];

			if (file[0] == '-' && file[1] == 0) {
				copyOut(STDIN, STDIN_FILENO);
			} else {
				fd = open(file, O_RDONLY);
				if (fd < 0) {
					exitCode = errno;
					reportStdErr(prog, file);
				} else {
					if (fstat(fd, &st) != 0) {
						exitCode = errno;
						reportStdErr(prog, file);
					}
					if (S_ISREG(st.st_mode)) {
						copyOut(file, fd);
					} else if (S_ISDIR(st.st_mode)) {
						stdErr(prog);
						stdErr(": ");
						stdErr(file);
						stdErr(": ");
						stdErr("Is a directory\n");
					}
					close(fd);
				}
			}
		}
	}
	output(outBuf, outp - outBuf);
	return exitCode;
}

