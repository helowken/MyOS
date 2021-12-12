#include "pm.h"
#include "string.h"

void panic(char *who, char *msg, int num) {
	// TODO
}

char *findParam(const char *name) {
	register const char *namep;
	register char *envp;

	for (envp = (char *) monitorParams; *envp != 0;) {
		for (namep = name; *namep != 0 && *namep == *envp; ++namep, ++envp) {
		}
		if (*namep == '\0' && *envp == '=')
		  return envp + 1;
		while (*envp++ != 0) {
		}
	}
	return NULL;
}
