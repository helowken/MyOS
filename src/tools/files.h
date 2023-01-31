/* Users */
#define ROOT	SUPER_USER
#define BIN		2

/* Groups */
#define	GOP		0	/* opeartor */
#define GBIN	2	
#define GTTY	4
#define GKMEM	8

/* File types */
#define I_B		I_BLOCK_SPECIAL
#define I_C		I_CHAR_SPECIAL
#define I_R		I_REGULAR
#define	I_D		I_DIRECTORY

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
	/* c[0-3]d[0-7]p[0-3]s[0-3] */
	{ "c0d0p0s0",     I_B | 0600, ROOT, GOP,   RDEV(3, 0x80) },
	{ "c0d0p0s1",     I_B | 0600, ROOT, GOP,   RDEV(3, 0x81) },
	{ "c0d0p0s2",     I_B | 0600, ROOT, GOP,   RDEV(3, 0x82) },

	/* CTTY */
	{ "tty",          I_C | 0666, ROOT, GOP,   RDEV(CTTY_MAJOR, 0x00) },

	/* TTY */
	{ "console",      I_C | 0620, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x00) },
	{ "log",          I_C | 0222, ROOT, GOP,   RDEV(TTY_MAJOR, 0x0F) },
	{ "pty0",         I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0xC0) },
	{ "pty1",         I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0xC1) },
	{ "pty2",         I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0xC2) },
	{ "pty3",         I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0xC3) },
	{ "tty00",        I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x10) },
	{ "tty01",        I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x11) },
	{ "tty02",        I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x12) },
	{ "tty03",        I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x13) },
	{ "ttyc1",        I_C | 0600, ROOT, GOP,   RDEV(TTY_MAJOR, 0x01) },
	{ "ttyc2",        I_C | 0600, ROOT, GOP,   RDEV(TTY_MAJOR, 0x02) },
	{ "ttyc3",        I_C | 0600, ROOT, GOP,   RDEV(TTY_MAJOR, 0x03) },
	{ "ttyp0",        I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x80) },
	{ "ttyp1",        I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x81) },
	{ "ttyp2",        I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x82) },
	{ "ttyp3",        I_C | 0666, ROOT, GTTY,  RDEV(TTY_MAJOR, 0x83) },

	/* MEMORY */
	{ "ram",          I_B | 0600, ROOT, GKMEM, RDEV(MEMORY_MAJOR, RAM_DEV) },
	{ "mem",          I_C | 0640, ROOT, GKMEM, RDEV(MEMORY_MAJOR, MEM_DEV) },
	{ "kmem",         I_C | 0640, ROOT, GKMEM, RDEV(MEMORY_MAJOR, KMEM_DEV) },
	{ "null",         I_C | 0666, ROOT, GKMEM, RDEV(MEMORY_MAJOR, NULL_DEV) },
	{ "boot",         I_B | 0600, ROOT, GKMEM, RDEV(MEMORY_MAJOR, BOOT_DEV) },
	{ "zero",         I_C | 0644, ROOT, GKMEM, RDEV(MEMORY_MAJOR, ZERO_DEV) },

	/* LOG */
	{ "klog",         I_C | 0600, ROOT, GOP,   RDEV(LOG_MAJOR, 0x00) },

	/* CMOS */
	{ "cmos",         I_C | 0600, ROOT, GOP,   RDEV(0x11, 0x00) },
	{ NULL }
};

static FileInfo sbinFiles[] = {
	{ "cmos",         I_R | 0755, BIN,  GOP,   0, NULL, "sbin/cmos.bin" },
	{ NULL }
};

static FileInfo binFiles[] = {
	{ "sh",           I_R | 0755, BIN,  GOP,   0, NULL, "bin/sh.bin" },
	{ "loadkeys",     I_R | 0755, BIN,  GOP,   0, NULL, "bin/loadkeys.bin" },
	{ "sysenv",       I_R | 0755, BIN,  GOP,   0, NULL, "bin/sysenv.bin" },
	{ "expr",	      I_R | 0755, BIN,  GOP,   0, NULL, "bin/expr.bin", "test,[" },
	{ "echo",	      I_R | 0755, BIN,  GOP,   0, NULL, "bin/echo.bin" },
	{ "service",      I_R | 0755, BIN,  GOP,   0, NULL, "bin/service.bin" },
	{ "testAAA",      I_R | 0755, BIN,  GOP,   0, NULL, "bin/testAAA.bin" },//TODO
	{ "readclock",    I_R | 0755, BIN,  GOP,   0, NULL, "bin/readclock.bin" },
	{ "intr",         I_R | 0755, BIN,  GOP,   0, NULL, "bin/intr.bin" },
	{ "date",         I_R | 0755, BIN,  GOP,   0, NULL, "bin/date.bin" },
	{ "cat",          I_R | 0755, BIN,  GOP,   0, NULL, "bin/cat.bin" },
	{ "printroot",    I_R | 0755, BIN,  GOP,   0, NULL, "bin/printroot.bin" },
	{ "mount",        I_R | 0755, BIN,  GOP,   0, NULL, "bin/mount.bin" },
	{ "umount",       I_R | 0755, BIN,  GOP,   0, NULL, "bin/umount.bin" },
	//{ "pwd",		  I_R | 0755, BIN,  GOP,   0, NULL, "bin/pwd.bin" },
	
	/* TODO below are in /usr/bin/ */
	{ "sleep",        I_R | 0755, BIN,  GOP,   0, NULL, "bin/sleep.bin" },
	{ "stat",         I_R | 0755, BIN,  GOP,   0, NULL, "bin/stat.bin" },
	{ NULL }
};

static FileInfo etcFiles[] = {
	{ "rc",           I_R | 0755, ROOT, GOP,   0, NULL, "etc/rc" },
	{ "fstab",        I_R | 0755, ROOT, GOP,   0, NULL, "etc/fstab" },
	{ "keymap",       I_R | 0644, BIN,  GOP,   0, NULL, "etc/us-std.map" },
	{ NULL }
};

static FileInfo dirs[] = {
	{ "dev",          I_D | 0755, ROOT, GOP,   0, devFiles },
	{ "sbin",         I_D | 0755, BIN,  GOP,   0, sbinFiles },
	{ "bin",          I_D | 0755, BIN,  GOP,   0, binFiles },
	{ "etc",          I_D | 0755, ROOT, GOP,   0, etcFiles },
	{ "tmp",          I_D | 0777, ROOT, GOP,   0 },
	{ "usr",          I_D | 0777, BIN,  GBIN,  0 },
	{ "home",         I_D | 0777, BIN,  GBIN,  0 },
	{ NULL }
};

static FileInfo rootDir =
    { NULL,           I_D | 0755, ROOT, GOP,   0, dirs };



