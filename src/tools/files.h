#define SU				SUPER_USER
#define	ROOT_GRP		0
#define BIN				2
#define BIN_GRP			2
#define TTY_GRP			4
#define KMEM_GRP		8
#define RDEV(major, minor)	( (((major) & BYTE) << MAJOR) | \
								(((minor) & BYTE) << MINOR) )

typedef struct FileInfo {
	char *name;
	Mode_t mode;
	int uid;
	int gid;
	Dev_t rdev;
	struct FileInfo *files;
	char *path;
	char *linkNames;
} FileInfo;

static FileInfo devFiles[] = {
	/* CTTY */
	{ "tty",     I_CHAR_SPECIAL  | 0666,  SU, ROOT_GRP,  RDEV(CTTY_MAJOR, 0x0) },

	/* TTY */
	{ "console", I_CHAR_SPECIAL  | 0620,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x00) },
	{ "log",     I_CHAR_SPECIAL  | 0222,  SU, ROOT_GRP,  RDEV(TTY_MAJOR, 0x0F) },
	{ "pty0",    I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0xC0) },
	{ "pty1",    I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0xC1) },
	{ "pty2",    I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0xC2) },
	{ "pty3",    I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0xC3) },
	{ "tty00",   I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x10) },
	{ "tty01",   I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x11) },
	{ "tty02",   I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x12) },
	{ "tty03",   I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x13) },
	{ "ttyc1",   I_CHAR_SPECIAL  | 0600,  SU, ROOT_GRP,  RDEV(TTY_MAJOR, 0x01) },
	{ "ttyc2",   I_CHAR_SPECIAL  | 0600,  SU, ROOT_GRP,  RDEV(TTY_MAJOR, 0x02) },
	{ "ttyc3",   I_CHAR_SPECIAL  | 0600,  SU, ROOT_GRP,  RDEV(TTY_MAJOR, 0x03) },
	{ "ttyp0",   I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x80) },
	{ "ttyp1",   I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x81) },
	{ "ttyp2",   I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x82) },
	{ "ttyp3",   I_CHAR_SPECIAL  | 0666,  SU, TTY_GRP,   RDEV(TTY_MAJOR, 0x83) },

	/* MEMORY */
	{ "ram",     I_BLOCK_SPECIAL | 0600,  SU, KMEM_GRP,  RDEV(MEMORY_MAJOR, RAM_DEV) },
	{ "mem",     I_CHAR_SPECIAL  | 0640,  SU, KMEM_GRP,  RDEV(MEMORY_MAJOR, MEM_DEV) },
	{ "kmem",    I_CHAR_SPECIAL  | 0640,  SU, KMEM_GRP,  RDEV(MEMORY_MAJOR, KMEM_DEV) },
	{ "null",    I_CHAR_SPECIAL  | 0666,  SU, KMEM_GRP,  RDEV(MEMORY_MAJOR, NULL_DEV) },
	{ "boot",    I_BLOCK_SPECIAL | 0600,  SU, KMEM_GRP,  RDEV(MEMORY_MAJOR, BOOT_DEV) },
	{ "zero",    I_CHAR_SPECIAL  | 0644,  SU, KMEM_GRP,  RDEV(MEMORY_MAJOR, ZERO_DEV) },

	/* LOG */
	{ "klog",    I_CHAR_SPECIAL  | 0600,  SU, ROOT_GRP,  RDEV(LOG_MAJOR, 0x00) },

	/* CMOS */
	{ "cmos",    I_CHAR_SPECIAL  | 0600,  SU, ROOT_GRP,  RDEV(0x11, 0x00) },
	{ NULL }
};

static FileInfo sbinFiles[] = {
	{ "cmos",       I_REGULAR | 0755, BIN, ROOT_GRP, 0, NULL, "sbin/cmos.bin" },
	{ NULL }
};

static FileInfo binFiles[] = {
	{ "sh",       I_REGULAR | 0755, BIN, ROOT_GRP, 0, NULL, "bin/sh.bin" },
	{ "loadkeys", I_REGULAR | 0755, BIN, ROOT_GRP, 0, NULL, "bin/loadkeys.bin" },
	{ "sysenv",   I_REGULAR | 0755, BIN, ROOT_GRP, 0, NULL, "bin/sysenv.bin" },
	{ "expr",	  I_REGULAR | 0755, BIN, ROOT_GRP, 0, NULL, "bin/expr.bin",	   "test,[" },
	{ "service",  I_REGULAR | 0755, BIN, ROOT_GRP, 0, NULL, "bin/service.bin" },
	{ "testAAA",  I_REGULAR | 0755, BIN, ROOT_GRP, 0, NULL, "bin/testAAA.bin" },
	{ NULL }
};

static FileInfo etcFiles[] = {
	{ "rc",      I_REGULAR | 0755, SU,  ROOT_GRP, 0, NULL, "etc/rc" },
	{ "keymap",  I_REGULAR | 0644, BIN, ROOT_GRP, 0, NULL, "etc/us-std.map" },
	{ NULL }
};

static FileInfo dirs[] = {
	{ "dev",     I_DIRECTORY | 0755, SU,  ROOT_GRP, 0, devFiles },
	{ "sbin",    I_DIRECTORY | 0755, BIN, ROOT_GRP, 0, sbinFiles },
	{ "bin",     I_DIRECTORY | 0755, BIN, ROOT_GRP, 0, binFiles },
	{ "etc",     I_DIRECTORY | 0755, SU,  ROOT_GRP, 0, etcFiles },
	{ NULL }
};

static FileInfo rootDir =
    { NULL,      I_DIRECTORY | 0755, SU,  ROOT_GRP, 0, dirs };



