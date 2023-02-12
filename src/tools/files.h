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
	char *path;
	char *linkNames;
	struct FileInfo *files[10];		/* We assume 10 is the max size. */
} FileInfo;

typedef struct {
	char *fs_type;
	FileInfo *fs_info;
} FS;


/* /bin and /usr/bin share the same bin files */
static FileInfo commonBinFiles[] = {
	{ "sh",           I_R | 0755, BIN,  GOP,   0, "commands/ash/sh.bin" },
	{ "expr",	      I_R | 0755, BIN,  GOP,   0, "commands/ash/bltin/expr.bin", "test,[" },
	{ "echo",	      I_R | 0755, BIN,  GOP,   0, "commands/ash/bltin/echo.bin" },
	{ "loadkeys",     I_R | 0755, BIN,  GOP,   0, "commands/ibm/loadkeys.bin" },
	{ "readclock",    I_R | 0755, BIN,  GOP,   0, "commands/ibm/readclock.bin" },
	{ "sysenv",       I_R | 0755, BIN,  GOP,   0, "commands/simple/sysenv.bin" },
	{ "intr",         I_R | 0755, BIN,  GOP,   0, "commands/simple/intr.bin" },
	{ "date",         I_R | 0755, BIN,  GOP,   0, "commands/simple/date.bin" },
	{ "cat",          I_R | 0755, BIN,  GOP,   0, "commands/simple/cat.bin" },
	{ "printroot",    I_R | 0755, BIN,  GOP,   0, "commands/simple/printroot.bin" },
	{ "mount",        I_R | 0755, BIN,  GOP,   0, "commands/simple/mount.bin" },
	{ "umount",       I_R | 0755, BIN,  GOP,   0, "commands/simple/umount.bin" },
	{ "pwd",		  I_R | 0755, BIN,  GOP,   0, "commands/simple/pwd.bin" },
	{ NULL }
};

/* ---- /root dir and files ---- */
static FileInfo rootDevFiles[] = {
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

static FileInfo rootSbinFiles[] = {
	{ "cmos",         I_R | 0755, BIN,  GOP,   0, "drivers/cmos/cmos.bin" },
	{ NULL }
};

static FileInfo rootBinFiles[] = {
	{ "service",      I_R | 0755, BIN,  GOP,   0, "servers/rs/service.bin" },
	{ "testAAA",      I_R | 0755, BIN,  GOP,   0, "commands/simple/testAAA.bin" },//TODO
	{ "cp",		      I_R | 0755, BIN,  GOP,   0, "commands/simple/cp.bin", "mv,ln,rm" },
	{ NULL }
};

static FileInfo rootEtcFiles[] = {
	{ "rc",           I_R | 0755, ROOT, GOP,   0, "tools/res/root/etc/rc" },
	{ "fstab",        I_R | 0755, ROOT, GOP,   0, "tools/res/root/etc/fstab" },
	{ "passwd",       I_R | 0755, ROOT, GOP,   0, "tools/res/root/etc/passwd" },
	{ "group",        I_R | 0755, ROOT, GOP,   0, "tools/res/root/etc/group" },
	{ "keymap",       I_R | 0644, BIN,  GOP,   0, "drivers/tty/keymaps/us-std.map" },
	{ NULL }
};

static FileInfo rootDirs[] = {
	{ "dev",          I_D | 0755, ROOT, GOP,   0, NULL, NULL, { rootDevFiles, NULL } },
	{ "sbin",         I_D | 0755, BIN,  GOP,   0, NULL, NULL, { rootSbinFiles, NULL } },
	{ "bin",          I_D | 0755, BIN,  GOP,   0, NULL, NULL, { commonBinFiles, rootBinFiles,  NULL } },
	{ "etc",          I_D | 0755, ROOT, GOP,   0, NULL, NULL, { rootEtcFiles, NULL } },
	{ "tmp",          I_D | 0777, ROOT, GOP,   0 },
	{ "usr",          I_D | 0755, ROOT, GOP,   0 },
	{ "home",         I_D | 0755, ROOT, GOP,   0 },
	{ NULL }
};

/* ---- /usr dir and files ---- */
static FileInfo usrBinFiles[] = {
	{ "sleep",        I_R | 0755, BIN,  GOP,   0, "commands/simple/sleep.bin" },
	{ "stat",         I_R | 0755, BIN,  GOP,   0, "commands/simple/stat.bin" },
	{ "ls",           I_R | 0755, BIN,  GOP,   0, "commands/simple/ls.bin" },
	{ "mkdir",        I_R | 0755, BIN,  GOP,   0, "commands/simple/mkdir.bin" },
	{ "chmod",        I_R | 0755, BIN,  GOP,   0, "commands/simple/chmod.bin" },
	{ "cp",		      I_R | 0755, BIN,  GOP,   0, "commands/simple/cp.bin", "clone,cpdir,mv,ln,rm" },
	{ NULL }
};

static FileInfo usrDirs[] = {
	{ "bin",          I_D | 0755, BIN,  GOP,   0, NULL, NULL, { commonBinFiles, usrBinFiles, NULL } },
	{ NULL }
};


/* ---- 1st level dirs ---- */
static FileInfo rootInfo =
    { NULL,           I_D | 0755, ROOT, GOP,   0, NULL, NULL, { rootDirs, NULL } };
static FileInfo homeInfo =
    { NULL,           I_D | 0777, ROOT, GOP,   0 };
static FileInfo usrInfo =
    { NULL,           I_D | 0777, ROOT, GOP,   0, NULL, NULL, { usrDirs, NULL } };


static FS fsList[] = {
	{ "ROOT", &rootInfo },
	{ "HOME", &homeInfo },
	{ "USR",  &usrInfo },
	{ NULL }
};

