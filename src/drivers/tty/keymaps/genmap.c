#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../../../include/minix/keymap.h"

#include KEY_SRC

uint8_t compressMap[4 + NR_SCAN_CODES * MAP_COLS * 9/8 * 2 + 1];

static void tell(const char *s) {
	write(STDERR_FILENO, s, strlen(s));
}

int main(void) {
	uint8_t *cm, *fb;
	uint16_t *km;
	int n;

/* fb has 8-bits, each bit indicates which key has a high byte
 * in the following 8 keys.
 *
 * For example:
 *	fb = 0100 0001
 * then the following keys:
 *  1B 2B 1B 1B  1B 1B 1B 2B	(2B means low byte + high byte)
 */

	/* Compress the keymap. */
	memcpy(compressMap, KEY_MAGIC, 4);
	cm = compressMap + 4;
	n = 8;	/* We need to setup the flag byte first */
	for (km = keyMap; km < keyMap + NR_SCAN_CODES * MAP_COLS; ++km) {
		if (n == 8) {
			/* Allocate a new flag byte. */
			fb = cm;
			*cm++ = 0;	/* Init the flag byte to '0000 0000'. */
			n = 0;
		}
		*cm++ = (*km & 0x00FF);		/* Low byte. */
		if (*km & 0xFF00) {		
			*cm++ = (*km >> 8);		/* High byte only when set. */
			*fb |= (1 << n);		/* Set a flag if so. */
		}
		++n;
	}

	/* Don't store trailing zeros. 
	 * For example, us-std.src has zeros from 97 to 127.
	 */
	while (cm > compressMap && cm[-1] == 0) {
		--cm;
	}

	/* Emit the compressed keymap. */
	if (write(STDOUT_FILENO, compressMap, cm - compressMap) < 0) {
		int savedErrno = errno;

		tell("gemmap: ");
		tell(strerror(savedErrno));
		tell("\n");
		exit(1);
	}
	exit(0);
}
