#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>
#include <minix/minlib.h>

#include <minix/config.h>
#include <minix/const.h>
#include <minix/type.h>
#include <../servers/fs/const.h>
#include <../servers/fs/type.h>
#include <../servers/fs/super.h>

#define IS_DISK(mode)	S_ISBLK(mode)
#define seteuid(uid)	setuid(uid)
#define setegid(gid)	setgid(gid)

//static char MTAB[] = "/etc/mtab";

typedef struct MTab {
	struct MTab *next;
	dev_t device;
	char *devName;
	char *mountPoint;
} MTab;

static MTab *mtab = NULL;
static int iFlag = 0;	/* Focus on inodes instead of blocks. */
static int PFlag = 0;	/* Posix standardf output. */
static int kFlag = 0;	/* Output in kilobytes instead of 512 byte units for -P. */
static int isTty;
static uid_t ruid, euid;
static gid_t rgid, egid;
static int unitSize;
static char *prog;

static void readMTab(char *type) {
/* Turn the mounted file table into a list. */
	MTab **amt = &mtab, *new;
	struct stat st;
	char devName[128], mountPoint[128], version[10], rwFlag[10];

	if (loadMTab(prog) < 0)
	  exit(1);

	while (getMTabEntry(devName, mountPoint, version, rwFlag), 
				devName[0] != 0) {
		if (strcmp(type, "dev") == 0) {
			if (strcmp(version, "3") != 0)
			  continue;
		}
		else {
			if (strcmp(type, version) != 0) 
			  continue;
		}

		/* Skip bad entries, can't complain about everything. */
		if (stat(devName, &st) < 0 || ! IS_DISK(st.st_mode))
		  continue;

		/* Make new list cell. */
		if ((new = (MTab *) malloc(sizeof(*new))) == NULL ||
			(new->devName = (char *) malloc(strlen(devName) + 1)) == NULL ||
			(new->mountPoint = (char *) malloc(strlen(mountPoint) + 1)) == NULL)
		  break;

		new->device = st.st_rdev;
		strcpy(new->devName, devName);
		strcpy(new->mountPoint, mountPoint);

		*amt = new;
		amt = &new->next;
		*amt = NULL;
	}
}

static bit_t bitCount(unsigned blocks, bit_t bits, int fd, int blockSize) {
	char *wptr;
	int i, b;
	bit_t busy;
	char *wlim;
	static char buf[MAX_BLOCK_SIZE];
	static char bitsInChar[(1 << CHAR_BIT)];

	/* Precalculate bitcount for each char. */
	if (bitsInChar[1] != 1) {
		for (b = (1 << 0); b < (1 << CHAR_BIT); b <<= 1) {
			for (i = 0; i < (1 << CHAR_BIT); ++i) {
				if (i & b)
				  ++bitsInChar[i];
			}
		}
	}

	/* Loop on blocks, read one at a time and counting bits. */
	busy = 0;
	for (i = 0; i < blocks && bits != 0; ++i) {
		if (read(fd, buf, blockSize) != blockSize)
		  return -1;

		wptr = &buf[0];
		if (bits >= CHAR_BIT * blockSize) {
			wlim = &buf[blockSize];
			bits -= CHAR_BIT * blockSize;
		} else {
			b = bits / CHAR_BIT;	/* Whole chars in map */
			wlim = &buf[b];
			bits -= b * CHAR_BIT;	/* Bits in last char, if any */
			b = *wlim & ((1 << bits) - 1);	/* Bit pattern from last ch */
			busy += bitsInChar[b];
			bits = 0;
		}

		/* Loop on the chars of a block. */
		while (wptr != wlim) {
			busy += bitsInChar[(*wptr++ & ((1 << CHAR_BIT) - 1))];
		}
	}
	
	return busy;
}

static MTab *searchTab(char *name) {
/* See what we can do with a user supplied name, there are five possibilities:
 * 1. It's a device and it is in the mtab: Return mtab entry.
 * 2. It's a device and it is not in the mtab: Return device mounted on "".
 * 3. It's a file and lives on a device in the mtab: Return mtab entry.
 * 4. It's a file and it's not a on an mtab device: Search /dev for the device
 *    and return this device as mounted on "???".
 * 5. It's junk: Return something df() will choke on.
 */
	static MTab unknown;
	static char devName[5 + NAME_MAX + 1] = "/dev/";
	MTab *mt;
	struct stat st;
	DIR *dp;
	struct dirent *ent;

	unknown.devName = name;
	unknown.mountPoint = "";

	if (stat(name, &st) < 0)
	  return &unknown;	/* Case 5. */

	unknown.device = IS_DISK(st.st_mode) ? st.st_rdev : st.st_dev;

	for (mt = mtab; mt != NULL; mt = mt->next) {
		if (unknown.device == mt->device)
		  return mt;	/* Case 1 & 3. */
	}

	if (IS_DISK(st.st_mode)) 
	  return &unknown;	/* Case 2. */

	if ((dp = opendir("/dev")) == NULL)
	  return &unknown;	/* Disaster. */

	while ((ent = readdir(dp)) != NULL) {
		if (ent->d_name[0] == '.')
		  continue;
		strcpy(devName + 5, ent->d_name);
		if (stat(devName, &st) >= 0 && 
				IS_DISK(st.st_mode) &&
				unknown.device == st.st_rdev) {
			unknown.devName = devName;
			unknown.mountPoint = "???";
			break;
		}
	}
	closedir(dp);
	return &unknown;	/* Case 4. */
}

/* (num / total) in percentages rounded up. */
#define percent(num, total)	((int) ((100L * (num) + ((total) - 1)) / (total)))

/* One must be careful printing all these _t types. */
#define L(n)	((long) (n))

static int df(const MTab *mt) {
	int fd;
	bit_t iCount, zCount;
	block_t totalBlocks, busyBlocks;
	int n, blockSize;
	SuperBlock super, *sp;

	/* Don't allow Joe User to df just any device. */
	seteuid(*mt->mountPoint == 0 ? ruid : euid);
	setegid(*mt->mountPoint == 0 ? rgid : egid);

	if ((fd = open(mt->devName, O_RDONLY)) < 0) {
		fprintf(stderr, "df: %s: %s\n", mt->devName, strerror(errno));
		return 1;
	}

	lseek(fd, (off_t) SUPER_BLOCK_BYTES, SEEK_SET);	/* Skip boot block */

	if (read(fd, (char *) &super, sizeof(super)) != (int) sizeof(super)) {
		fprintf(stderr, "df: Can't read super block of %s\n", mt->devName);
		close(fd);
		return 1;
	}

	sp = &super;
	if (sp->s_magic != SUPER_V3) {
		fprintf(stderr, "df: %s: Not a valid file system\n", mt->devName);
		close(fd);
		return 1;
	}
	blockSize = super.s_block_size;

	if (blockSize < MIN_BLOCK_SIZE || blockSize > MAX_BLOCK_SIZE) {
		fprintf(stderr, "df: %s: funny block size (%d)\n",
					mt->devName, blockSize);
		close(fd);
		return 1;
	}

	/* Skip rest of super block. (see IMAP_OFFSET) */
	lseek(fd, (off_t) blockSize * 2L, SEEK_SET);	

	iCount = bitCount(sp->s_imap_blocks, (bit_t) (sp->s_inodes + 1),
				fd, blockSize);
	if (iCount == -1) {
		fprintf(stderr, "df: Can't find bit maps of %s\n", mt->devName);
		close(fd);
		return 1;
	}
	--iCount;	/* There is no inode 0. */

	/* The first bit in the zone map corresponds with zone s_first_data_zone - 1
	 * This means that there are s_zones - (s_first_data_zone - 1) bits in the map
	 */
	zCount = bitCount(sp->s_zmap_blocks, 
				(bit_t) (sp->s_zones - (sp->s_first_data_zone - 1)), 
				fd, blockSize);
	if (zCount == -1) {
		fprintf(stderr, "df: Can't find bit maps of %s\n", mt->devName);
		close(fd);
		return 1;
	}
	/* Don't forget those zones before sp->s_first_data_zone - 1 */
	zCount += sp->s_first_data_zone - 1;

	totalBlocks = (block_t) sp->s_zones << sp->s_log_zone_size;
	busyBlocks = (block_t) zCount << sp->s_log_zone_size;

	busyBlocks *= (blockSize / 512) / (unitSize / 512);
	totalBlocks *= (blockSize / 512) / (unitSize / 512);

	/* Print results. */
	printf("%s", mt->devName);
	n = strlen(mt->devName);
	if (n > 15 && isTty) {
		putchar('\n');
		n = 0;
	}
	while (n < 15) {
		putchar(' ');
		++n;
	}

	if (! PFlag && ! iFlag) {
		printf(" %9ld  %9ld  %9ld %3d%%   %3d%%   %s\n",
			L(totalBlocks),
			L(totalBlocks - busyBlocks),
			L(busyBlocks),
			percent(busyBlocks, totalBlocks),
			percent(iCount, sp->s_inodes),
			mt->mountPoint
		);
	}
	if (! PFlag && iFlag) {
		printf(" %9ld  %9ld  %9ld %3d%%   %3d%%   %s\n",
			L(sp->s_inodes),
			L(sp->s_inodes - iCount),
			L(iCount),
			percent(iCount, sp->s_inodes),
			percent(busyBlocks, totalBlocks),
			mt->mountPoint
		);
	}
	if (PFlag && ! iFlag) {
		printf(" %9ld   %9ld  %9ld     %4d%%    %s\n",
			L(totalBlocks),
			L(totalBlocks - busyBlocks),
			totalBlocks - busyBlocks,
			percent(busyBlocks, totalBlocks),
			mt->mountPoint
		);
	}
	if (PFlag && iFlag) {
		printf(" %9ld   %9ld  %9ld     %4d%%    %s\n",
			L(sp->s_inodes),
			L(iCount),
			L(sp->s_inodes - iCount),
			percent(iCount, sp->s_inodes),
			mt->mountPoint
		);
	}
	close(fd);
	return 0;
}

void main(int argc, char **argv) {
	int i;
	MTab *mt;
	char *type = "dev";
	int ex = 0;
	int opt;

	prog = getProg(argv);

	while ((opt = getopt(argc, argv, "ikPt:")) != EOF) {
		switch (opt) {
			case 'i': iFlag = 1; break;
			case 'k': kFlag = 1; break;
			case 'P': PFlag = 1; break;
			case 't': 
				type = optarg;
				break;
			default:
				usage(prog, "[-ikP] [-t type] [device]...");
		}
	}

	isTty = isatty(STDOUT_FILENO);
	ruid = getuid();
	euid = geteuid();
	rgid = getgid();
	egid = getegid();

	readMTab(type);

	if (! PFlag || (PFlag && kFlag))
	  unitSize = 1024;
	else
	  unitSize = 512;

	if (PFlag) {
		printf(! iFlag ? "\
Filesystem    %4d-blocks      Used    Available  Capacity  Mounted on\n" : "\
Filesystem         Inodes       IUsed      IFree    %%IUsed    Mounted on\n",
			unitSize);
	} else {
		printf("%s\n", ! iFlag ? "\
Filesystem      Size (kB)       Free       Used    % Files%   Mounted on" : "\
Filesystem          Files       Free       Used    % BUsed%   Mounted on");
	}

	if (optind == argc) {
		for (mt = mtab; mt != NULL; mt = mt->next) {
			ex |= df(mt);
		}
	} else {
		for (i = optind; i < argc; ++i) {
			ex |= df(searchTab(argv[i]));
		}
	}
	exit(ex);
}
