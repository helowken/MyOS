#define nil	0
#include <sys/types.h>
#include <sys/stat.h>
#include <minix/minlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define USR_MODES	(S_ISUID | S_IRWXU)
#define GRP_MODES	(S_ISGID | S_IRWXG)
#define EXE_MODES	(S_IXUSR | S_IXGRP | S_IXOTH)
#define ALL_MODES	(USR_MODES | GRP_MODES | S_IRWXO | S_ISVTX)

static char *prog;
static char *_symbolic;
static mode_t u_mask;
static struct stat st;

static void usageErr();

/* Parse a P1003.2 4.7.7-conformant symbolic mode. 
 *
 * symbolic format:
 *	- OCTAL-MODE 
 *	- [ugoa...][[-+=][perms...]...] 
 *    (perms: rwxXst or a single letter from the set ugo.)
 *
 * Multiple symbolic modes can be given, separated by comma.
 */

static int getOctalMode(char *symbolic, mode_t *mp) {
	char *end;
	unsigned long octalMode;

	octalMode = strtoul(symbolic, &end, 010);
	if (octalMode < ALL_MODES && *end == 0 && end != symbolic) {
		*mp = octalMode;
		return 0;
	}
	return -1;
}

static mode_t doParseMode(char *symbolic, mode_t oldMode) {
	mode_t who, mask, newMode, tmpMask;
	register char c;
	char action;

	newMode = oldMode & ALL_MODES;
	while (*symbolic) {
		/* Parse who. If who is 0, the effect is as if (a) were given,
		 * but bits that are set in the umask are not affected. 
		 */ 
		who = 0;
		for (; (c = *symbolic); ++symbolic) {	
			switch (c) {
				case 'a':
					who |= ALL_MODES;
					continue;
				case 'u':
					who |= USR_MODES;
					continue;
				case 'g':
					who |= GRP_MODES;
					continue;
				case 'o':
					who |= S_IRWXO;
					continue;
			}
			break;
		}
		if (c == 0 || c == ',')
		  usageErr();
		while ((c = *symbolic)) {
			if (c == ',')	/* Skip separator: u+x,g-w */
			  break;
			switch (c) {	/* Parse action */
				case '+':
				case '-':
				case '=':
					action = c;
					++symbolic;
					break;
				default:
					usageErr();
			}
			mask = 0;
			for (; (c = *symbolic); ++symbolic) {	/* Parse perms */
				switch (c) {
					case 'u':	/* Permissions granted to the owner */
						tmpMask = newMode & S_IRWXU;
						mask |= tmpMask | (tmpMask >> 3) | (tmpMask >> 6);
						++symbolic;
						break;
					case 'g':	/* Permissions granted to the group */
						tmpMask = newMode & S_IRWXG;
						mask |= tmpMask | (tmpMask >> 3) | (tmpMask << 3);
						++symbolic;
						break;
					case 'o':	/* Permissions granted to the other */
						tmpMask = newMode & S_IRWXO;
						mask |= tmpMask | (tmpMask << 3) | (tmpMask << 6);
						++symbolic;
						break;
					case 'r':	
						mask |= S_IRUSR | S_IRGRP | S_IROTH;
						continue;
					case 'w':
						mask |= S_IWUSR | S_IWGRP | S_IWOTH;
						continue;
					case 'x':
						mask |= EXE_MODES;
						continue;
					case 's':
						mask |= S_ISUID | S_ISGID;
						continue;
					case 'X':	/* Dir or already has exec perms for some user */
						if (S_ISDIR(oldMode) || (oldMode & EXE_MODES))
						  mask |= EXE_MODES;
						continue;
					case 't':	/* Sticky bit */
						mask |= S_ISVTX;
						who |= S_ISVTX;
						continue;
				}
				break;
			}
			switch (action) {
				case '=':
					if (who)
					  newMode &= ~who;
					else
					  newMode = 0;
					/* Fall through */
				case '+':	/* Add to the existing file mode bits */
					if (who)
					  newMode |= who & mask;
					else
					  newMode |= mask & (~u_mask);
					break;
				case '-':
					if (who)
					  newMode &= ~(who & mask);
					else
					  newMode &= ~mask | u_mask;
			}
		}
		if (c) 
		  ++symbolic;
	}
	return newMode;
}
