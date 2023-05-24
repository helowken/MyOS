#include "sysutil.h"
#include <minix/config.h>
#include <string.h>

static int argc = 0;
static char **argv = NULL;

void envSetArgs(int sArgc, char *sArgv[]) {
	argc = sArgc;
	argv = sArgv;
}

static char *findKey(const char *params, const char *name) {
	register const char *np;
	register char *ep;

	for (ep = (char *) params; *ep != 0; ) {
		for (np = name; *np != 0 && *np == *ep; ++np, ++ep) {
		}
		if (*np == 0 && *ep == '=')
		  return ep + 1;
		while (*ep++ != 0) {
		}
	}
	return NULL;
}

int envGetParam(char *key, char *value, int maxSize) {
	Message msg;
	static char monParams[128 * sizeof(char *)];
	char *keyValue;
	int i, s, keyLen;

	if (key == NULL)
	  return EINVAL;

	keyLen = strlen(key);
	for (i = 1; i < argc; ++i) {
		if (strncmp(argv[i], key, keyLen) != 0)
		  continue;
		if (strlen(argv[i]) <= keyLen)
		  continue;
		if (argv[i][keyLen] != '=')
		  continue;
		keyValue = argv[i] + keyLen + 1;
		if (strlen(keyValue) + 1 > EP_BUF_SIZE)
		  return E2BIG;
		strcpy(value, keyValue);
		return OK;
	}

	/* Get copy of boot monitor parameters. */
	msg.m_type = SYS_GETINFO;
	msg.I_REQUEST = GET_MONPARAMS;
	msg.I_PROC_NR = SELF;
	msg.I_VAL_LEN = sizeof(monParams);
	msg.I_VAL_PTR = monParams;
	if ((s = taskCall(SYSTASK, SYS_GETINFO, &msg)) != OK) {
		printf("SYS_GETINFO: %d (size %u)\n", s, sizeof(monParams));
		return s;
	}

	/* We got a copy, now search requested key. */
	if ((keyValue = findKey(monParams, key)) == NULL)
	  return ESRCH;

	/* Value found, make the actual copy (as for as possible). */
	strncpy(value, keyValue, maxSize);

	/* See if it fits in the client's buffer. */
	if ((strlen(keyValue) + 1) > maxSize)
	  return E2BIG;

	return OK;
}
