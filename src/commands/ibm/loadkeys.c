#include <sys/types.h>
#include <sys/ioctl.h>
#include <minix/keymap.h>
#include <minix/minlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define KBD_DEVICE	"/dev/console"

static u16_t keyMap[NR_SCAN_CODES * MAP_COLS];
static u8_t compressMap[4 + NR_SCAN_CODES * MAP_COLS * 9/8 * 2 + 1];
static char *prog;

int main(int argc, char *argv[]) {
	u8_t *cm;
	u16_t *km;
	int fd, n, fb;

	prog = getProg(argv);
	if (argc != 2) 
	  usage(prog, "mapfile");

	if ((fd = open(argv[1], O_RDONLY)) < 0 || 
			read(fd, compressMap, sizeof(compressMap)) < 0)
	  fatal(prog, argv[1]);

	if (memcmp(compressMap, KEY_MAGIC, 4) != 0) {
		stdErr("loadkeys: ");
		stdErr(argv[1]);
		stdErr(": not a keymap file\n");
		exit(1);
	}
	close(fd);

	/* decompress the keymap data. */
	cm = compressMap + 4;
	n = 8;
	for (km = keyMap; km < keyMap + NR_SCAN_CODES * MAP_COLS; ++km) {
		if (n == 8) {
			/* Need a new flag byte. */
			fb = *cm++;
			n = 0;
		}
		*km = *cm++;	/* Load byte. */
		if (fb & (1 << n)) {
			*km |= (*cm++ << 8);	/* One of the few special keys. */
		}
		++n;
	}

	if ((fd = open(KBD_DEVICE, O_WRONLY)) < 0)
	  fatal(prog, KBD_DEVICE);

	if (ioctl(fd, KIOCSMAP, keyMap) < 0)
	  fatal(prog, KBD_DEVICE);

	return 0;
}
