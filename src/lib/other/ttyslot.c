#define _MINIX_SOURCE
#include <sys/types.h>
#include <ttyent.h>
#include <string.h>
#include <unistd.h>

int ttyslot() {
	int slot;

	slot = fttyslot(0);
	if (slot == 0)
	  slot = fttyslot(1);
	if (slot == 0)
	  slot = fttyslot(2);
	return slot;
}

int fttyslot(int fd) {
	char *ttyName;
	int lineNum;
	struct ttyent *ttyp;

	ttyName = ttyname(fd);
	if (ttyName == NULL)
	  return 0;

	/* Assume that tty devices are in /dev */
	if (strncmp(ttyName, "/dev/", 5) != 0)
	  return 0;		/* Malformed tty name. */

	ttyName += 5;

	/* Scan /etc/ttytab. */
	lineNum = 1;
	while ((ttyp = getttyent()) != NULL) {
		if (strcmp(ttyName, ttyp->ty_name) == 0) {
			endttyent();
			return lineNum;
		}
		++lineNum;
	}
	/* No match */
	endttyent();
	return 0;
}
