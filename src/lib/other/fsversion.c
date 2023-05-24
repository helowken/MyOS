#include <sys/types.h>
#include <stdint.h>
#include <minix/config.h>
#include <minix/const.h>
#include <minix/minlib.h>
#include <minix/type.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "fs/const.h"
#include "fs/type.h"
#include "fs/super.h"

static SuperBlock super, *sp;

int fsVersion(char *dev, char *prog) {
	int fd;

	if ((fd = open(dev, O_RDONLY)) < 0) {
		stdErr(prog);
		stdErr(" cannot open ");
		perror(dev);
		return -1;
	}

	lseek(fd, (off_t) SUPER_BLOCK_BYTES, SEEK_SET);		/* Skip boot block */
	if (read(fd, (char *) &super, (unsigned) SUPER_SIZE) != SUPER_SIZE) {
		stdErr(prog);
		stdErr(" cannot read super block on ");
		perror(dev);
		close(fd);
		return -1;
	}
	close(fd);

	sp = &super;
	if (sp->s_magic == SUPER_V3)
	  return 3;
	return -1;
}
