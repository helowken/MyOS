#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "sys/stat.h"
#include "minix/ipc.h"
#include "minix/const.h"
#include "minix/com.h"
#include "minix/type.h"
#include "minix/syslib.h"

/* This array defines all known requests. */
static char *knownRequests[] = {
	"up",
	"down",
	"catch for illegal requests"
};
#define ILLEGAL_REQUEST		sizeof(knownRequests) / sizeof(char *)

#define OK	0

/* Define names for arguments provided to this utility. The first few
 * arguments are required and have a known index. Thereafter, some optional
 * argument pairs like "-args argList" follow.
 */
#define ARG_NAME		0	/* Own application name */
#define ARG_REQUEST		1	/* Request to perform */
#define ARG_PATH		2	/* Binary of system service */

#define MIN_ARG_COUNT	3	/* Minimum number of arguments */

#define ARG_ARGS	"-args"		/* List of arguments to be passed */
#define ARG_DEV		"-dev"		/* Major device number for drivers */
#define ARG_PRIV	"-priv"		/* Required privileges */

/* The function parseArguments() verifies and parses the command line
 * parameters passed to this utility. Request parameters that are needed
 * are stored globally in the following variables:
 */
static int reqType;
static char *reqPath;
static char *reqArgs;
static int reqMajor;
static char *reqPriv;

/* An error occurred. Report the problem, print the usage, and exit.
 */
static void printUsage(char *appName, char *problem) {
	printf("Warning, %s\n", problem);
	printf("Usage:\n");
	printf("	%s <request> <binary> [%s <args>] [%s <special>]\n",
				appName, ARG_ARGS, ARG_DEV);
	printf("\n");
}

/* An unexpected, unrecoverable error occurred. Report and exit.
 */
static void panic(char *appName, char *msg, int num) {
	printf("Panic in %s: %s", appName, msg);
	if (num != NO_NUM)
	  printf(": %d", num);
	printf("\n");
	exit(EGENERIC);
}

static void parseArguments(int argc, char **argv) {
	struct stat st;
	int i;

	/* Verify argument count. */
	if (argc < MIN_ARG_COUNT) {
		printUsage(argv[ARG_NAME], "wrong number of arguments");
		exit(EINVAL);
	}

	/* Verify request type. */
	for (reqType = 0; reqType < ILLEGAL_REQUEST; ++reqType) {
		if (strcmp(knownRequests[reqType], argv[ARG_REQUEST]) == 0)
		  break;
	}
	if (reqType == ILLEGAL_REQUEST) {
		printUsage(argv[ARG_NAME], "illegal request type");
		exit(ENOSYS);
	}

	/* Verify the name of the binary of the system service. */
	reqPath = argv[ARG_PATH];
	if (reqPath[0] != '/') {
		printUsage(argv[ARG_NAME], "binary should be absolute path");
		exit(EINVAL);
	}
	if (stat(reqPath, &st) == -1) {
		printUsage(argv[ARG_NAME], "couldn't get status of binary");
		exit(errno);
	}
	if (! (st.st_mode & S_IFREG)) {
		printUsage(argv[ARG_NAME], "binary is not a regular file");
		exit(EINVAL);
	}

	/* Check optional arguments that come in pairs like "-args argList." */
	for (i = MIN_ARG_COUNT; i < argc; i += 2) {
		if (i + 1 >= argc) {
			printUsage(argv[ARG_NAME], "optional argument not complete");
			exit(EINVAL);
		}
		if (strcmp(argv[i], ARG_ARGS) == 0) {
			reqArgs = argv[i + 1];
		} else if (strcmp(argv[i], ARG_DEV) == 0) {
			if (stat(argv[i + 1], &st) == -1) {
				printUsage(argv[ARG_NAME], "couldn't get status of device node");
				exit(errno);
			}
			if (! (st.st_mode & (S_IFBLK | S_IFCHR))) {
				printUsage(argv[ARG_NAME], "special file is not a device node");
				exit(EINVAL);
			}
			reqMajor = MAJOR_DEV(st.st_rdev);
		} else if (strcmp(argv[i], ARG_PRIV) == 0) {
			reqPriv = argv[i + 1];	
		} else {
			printUsage(argv[ARG_NAME], "unknown optional argument given");
			exit(EINVAL);
		}
	}
}

int main(int argc, char **argv) {
	Message msg;
	int result;
	int s;

	/* Verify and parse the command line arguments. All arguments are checked
	 * here. If an error occurs, the problem is reported and exit(2) is called.
	 * All needed parameters to perform the request are extracted and stored
	 * global variables.
	 */
	parseArguments(argc, argv);

	/* Arguments seen fine. Try to perform the request. Only valid requests
	 * should end up here. The default is used for not yet supported requests.
	 */
	switch (reqType + SRV_REQ_BASE) {
		case SRV_UP:
			msg.SRV_PATH_ADDR = reqPath;
			msg.SRV_PATH_LEN = strlen(reqPath);
			msg.SRV_ARGS_ADDR = reqArgs;
			msg.SRV_ARGS_LEN = strlen(reqArgs);
			msg.SRV_DEV_MAJOR = reqMajor;
			if ((s = taskCall(RS_PROC_NR, SRV_UP, &msg)) != OK)
			  panic(argv[ARG_NAME], "sendRec to manager server failed", s);
			result = msg.m_type;
			break;
		case SRV_DOWN:
		case SRV_STATUS:
		default:
			printUsage(argv[ARG_NAME], "request is not yet supported");
			result = EGENERIC;
	}
	return result;
}
