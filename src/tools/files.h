/* Users */
#define ROOT	SUPER_USER
#define DAEM	1	/* Daemon */
#define BIN		2

/* Groups */
#define	GOP		0	/* opeartor */
#define GDAEM	1	/* Daemon */
#define GBIN	2	
#define GTTY	4
#define GUUCP	5
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
	{ "sh",           I_R | 0755,  BIN,  GOP,   0, "commands/ash/sh.bin" },
	{ "expr",	      I_R | 0755,  BIN,  GOP,   0, "commands/ash/bltin/expr.bin", "test,[" },
	{ "echo",	      I_R | 0755,  BIN,  GOP,   0, "commands/ash/bltin/echo.bin" },

	{ "loadkeys",     I_R | 0755,  BIN,  GOP,   0, "commands/ibm/loadkeys.bin" },
	{ "readclock",    I_R | 0755,  BIN,  GOP,   0, "commands/ibm/readclock.bin" },

	{ "sysenv",       I_R | 0755,  BIN,  GOP,   0, "commands/simple/sysenv.bin" },
	{ "intr",         I_R | 0755,  BIN,  GOP,   0, "commands/simple/intr.bin" },
	{ "date",         I_R | 0755,  BIN,  GOP,   0, "commands/simple/date.bin" },
	{ "cat",          I_R | 0755,  BIN,  GOP,   0, "commands/simple/cat.bin" },
	{ "printroot",    I_R | 0755,  BIN,  GOP,   0, "commands/simple/printroot.bin" },
	{ "mount",        I_R | 04755, ROOT, GOP,   0, "commands/simple/mount.bin" },
	{ "umount",       I_R | 04755, ROOT, GOP,   0, "commands/simple/umount.bin" },
	{ "pwd",		  I_R | 0755,  BIN,  GOP,   0, "commands/simple/pwd.bin" },
	{ "fsck",		  I_R | 0755,  BIN,  GOP,   0, "commands/simple/fsck.bin" },
	{ "getty",		  I_R | 0755,  BIN,  GOP,   0, "commands/simple/getty.bin" },
	{ NULL }
};

/* '/dev' */
static FileInfo rootDevFiles[] = {
	/* c[0-3]d[0-7]p[0-3]s[0-3] */
	{ "c0d0p0s0",     I_B | 0600,  ROOT, GOP,   RDEV(3, 0x80) },
	{ "c0d0p0s1",     I_B | 0600,  ROOT, GOP,   RDEV(3, 0x81) },
	{ "c0d0p0s2",     I_B | 0600,  ROOT, GOP,   RDEV(3, 0x82) },

	/* CTTY */
	{ "tty",          I_C | 0666,  ROOT, GOP,   RDEV(CTTY_MAJOR, 0x00) },

	/* TTY */
	{ "console",      I_C | 0620,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x00) },
	{ "log",          I_C | 0222,  ROOT, GOP,   RDEV(TTY_MAJOR, 0x0F) },
	{ "pty0",         I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0xC0) },
	{ "pty1",         I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0xC1) },
	{ "pty2",         I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0xC2) },
	{ "pty3",         I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0xC3) },
	{ "tty00",        I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x10) },
	{ "tty01",        I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x11) },
	{ "tty02",        I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x12) },
	{ "tty03",        I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x13) },
	{ "ttyc1",        I_C | 0600,  ROOT, GOP,   RDEV(TTY_MAJOR, 0x01) },
	{ "ttyc2",        I_C | 0600,  ROOT, GOP,   RDEV(TTY_MAJOR, 0x02) },
	{ "ttyc3",        I_C | 0600,  ROOT, GOP,   RDEV(TTY_MAJOR, 0x03) },
	{ "ttyp0",        I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x80) },
	{ "ttyp1",        I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x81) },
	{ "ttyp2",        I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x82) },
	{ "ttyp3",        I_C | 0666,  ROOT, GTTY,  RDEV(TTY_MAJOR, 0x83) },

	/* MEMORY */
	{ "ram",          I_B | 0600,  ROOT, GKMEM, RDEV(MEMORY_MAJOR, RAM_DEV) },
	{ "mem",          I_C | 0640,  ROOT, GKMEM, RDEV(MEMORY_MAJOR, MEM_DEV) },
	{ "kmem",         I_C | 0640,  ROOT, GKMEM, RDEV(MEMORY_MAJOR, KMEM_DEV) },
	{ "null",         I_C | 0666,  ROOT, GKMEM, RDEV(MEMORY_MAJOR, NULL_DEV) },
	{ "boot",         I_B | 0600,  ROOT, GKMEM, RDEV(MEMORY_MAJOR, BOOT_DEV) },
	{ "zero",         I_C | 0644,  ROOT, GKMEM, RDEV(MEMORY_MAJOR, ZERO_DEV) },

	/* LOG */
	{ "klog",         I_C | 0600,  ROOT, GOP,   RDEV(LOG_MAJOR, 0x00) },

	/* CMOS */
	{ "cmos",         I_C | 0600,  ROOT, GOP,   RDEV(0x11, 0x00) },

	/* RANDOM */
	{ "random",		  I_C | 0644,  ROOT, GOP,   RDEV(0x10, 0x00) },
	{ NULL }
};

/* '/sbin' */
static FileInfo rootSbinFiles[] = {
	{ "cmos",         I_R | 0755,  BIN,  GOP,   0, "drivers/cmos/cmos.bin" },
	{ NULL }
};

/* '/bin' */
static FileInfo rootBinFiles[] = {
	{ "service",      I_R | 0755,  BIN,  GOP,   0, "servers/rs/service.bin" },

	{ "cp",		      I_R | 0755,  BIN,  GOP,   0, "commands/simple/cp.bin", "mv,ln,rm" },

	{ "halt",         I_R | 0744,  ROOT, GOP,   0, "commands/reboot/tinyhalt.bin", "reboot" },
	{ NULL }
};

/* '/etc' */
static FileInfo rootEtcFiles[] = {
	{ "rc",           I_R | 0755,  ROOT, GOP,   0, "tools/res/root/etc/rc" },
	{ "fstab",        I_R | 0755,  ROOT, GOP,   0, "tools/res/root/etc/fstab" },
	{ "passwd",       I_R | 0755,  ROOT, GOP,   0, "tools/res/root/etc/passwd" },
	{ "shadow",       I_R | 0600,  ROOT, GOP,   0, "tools/res/root/etc/shadow" },
	{ "group",        I_R | 0755,  ROOT, GOP,   0, "tools/res/root/etc/group" },
	{ "hostname.file",I_R | 0755,  ROOT, GOP,   0, "tools/res/root/etc/hostname.file" },
	{ "ttytab",       I_R | 0755,  ROOT, GOP,   0, "tools/res/root/etc/ttytab" },
	{ "profile",      I_R | 0755,  ROOT, GOP,   0, "tools/res/root/etc/profile" },

	{ "keymap",       I_R | 0644,  BIN,  GOP,   0, "drivers/tty/keymaps/us-std.map" },
	{ NULL }
};

/* '/root' */
static FileInfo suFiles[] = {
	{ ".profile",     I_R | 0644,  BIN,  GOP,   0, "tools/res/users/root/.profile" },
	{ ".ashrc",		  I_R | 0644,  BIN,  GOP,   0, "tools/res/users/root/.ashrc" },
	{ NULL }
};

/* '/' */
static FileInfo rootDirs[] = {
	{ "dev",          I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { rootDevFiles, NULL } },
	{ "sbin",         I_D | 0755,  BIN,  GOP,   0, NULL, NULL, { rootSbinFiles, NULL } },
	{ "bin",          I_D | 0755,  BIN,  GOP,   0, NULL, NULL, { commonBinFiles, rootBinFiles,  NULL } },
	{ "etc",          I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { rootEtcFiles, NULL } },
	{ "tmp",          I_D | 0777,  ROOT, GOP,   0 },
	{ "usr",          I_D | 0755,  ROOT, GOP,   0 },
	{ "home",         I_D | 0755,  ROOT, GOP,   0 },
	/* user dirs */
	{ "root",         I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { suFiles, NULL } },
	{ NULL }
};

/* '/usr/bin' */
static FileInfo usrBinFiles[] = {
	{ "sleep",        I_R | 0755,  BIN,  GOP,   0, "commands/simple/sleep.bin" },
	{ "stat",         I_R | 0755,  BIN,  GOP,   0, "commands/simple/stat.bin" },
	{ "ls",           I_R | 0755,  BIN,  GOP,   0, "commands/simple/ls.bin" },
	{ "mkdir",        I_R | 0755,  BIN,  GOP,   0, "commands/simple/mkdir.bin" },
	{ "chmod",        I_R | 0755,  BIN,  GOP,   0, "commands/simple/chmod.bin" },
	{ "cp",		      I_R | 0755,  BIN,  GOP,   0, "commands/simple/cp.bin", "clone,cpdir,mv,ln,rm" },
	{ "uname",        I_R | 0755,  BIN,  GOP,   0, "commands/simple/uname.bin", "arch" },
	{ "touch",        I_R | 0755,  BIN,  GOP,   0, "commands/simple/touch.bin" },
	{ "dd",           I_R | 0755,  BIN,  GOP,   0, "commands/simple/dd.bin" },
	{ "od",           I_R | 0755,  BIN,  GOP,   0, "commands/simple/od.bin" },
	{ "at",           I_R | 04755, ROOT, GOP,   0, "commands/simple/at.bin" },
	{ "kill",         I_R | 0755,  BIN,  GOP,   0, "commands/simple/kill.bin" },
	{ "update",       I_R | 0755,  BIN,  GOP,   0, "commands/simple/update.bin" },
	{ "usyslogd",     I_R | 0755,  BIN,  GOP,   0, "commands/simple/usyslogd.bin" },
	{ "mail",         I_R | 04755, ROOT, GOP,   0, "commands/simple/mail.bin" },
	{ "login",        I_R | 0755,  BIN,  GOP,   0, "commands/simple/login.bin" },
	{ "env",          I_R | 0755,  BIN,  GOP,   0, "commands/simple/env.bin" },
	{ "printenv",     I_R | 0755,  BIN,  GOP,   0, "commands/simple/printenv.bin" },
	{ "which",		  I_R | 0755,  BIN,  GOP,   0, "commands/simple/which.bin" },
	{ "basename",	  I_R | 0755,  BIN,  GOP,   0, "commands/simple/basename.bin" },
	{ "dirname",	  I_R | 0755,  BIN,  GOP,   0, "commands/simple/dirname.bin" },
	{ "dirname",	  I_R | 0755,  BIN,  GOP,   0, "commands/simple/dirname.bin" },
	{ "last",	      I_R | 0755,  BIN,  GOP,   0, "commands/simple/last.bin", "uptime" },

	{ "rotate",       I_R | 0755,  BIN,  GOP,   0, "commands/scripts/rotate.sh" },
	{ "whereis",      I_R | 0755,  BIN,  GOP,   0, "commands/scripts/whereis.sh" },

	{ "shutdown",     I_R | 04754, ROOT, GOP,   0, "commands/reboot/shutdown.bin", "arch" },
	{ "halt",         I_R | 0744,  ROOT, GOP,   0, "commands/reboot/halt.bin", "reboot" },

	{ "cron",         I_R | 0755,  BIN,  GOP,   0, "commands/cron/cron.bin" },
	{ "crontab",      I_R | 04755, ROOT, GOP,   0, "commands/cron/crontab.bin" },
	{ NULL }
};

/* '/usr/sbin' */
static FileInfo usrSbinFiles[] = {
	{ "random",       I_R | 0755,  ROOT, GOP,   0, "drivers/random/random.bin" },
	{ NULL }
};

/* '/usr/etc' */
static FileInfo usrEtcFiles[] = {
	{ "rc",           I_R | 0755,  ROOT, GOP,   0, "tools/res/usr/etc/rc" },
	{ "daily",        I_R | 0755,  ROOT, GOP,   0, "tools/res/usr/etc/daily" },
	{ NULL }
};

/* '/usr/spool/at' */
static FileInfo atFiles[] = {
	{ "past",         I_D | 0711,  ROOT, GUUCP, 0 },
	{ NULL }
};

/* '/usr/spool' */
static FileInfo usrSpoolFiles[] = {
	{ "at",			  I_D | 0711,  ROOT, GUUCP, 0, NULL, NULL, { atFiles, NULL } },
	{ "crontabs",     I_D | 0700,  ROOT, GOP,   0 },
	{ "locks",        I_D | 0775,  ROOT, GUUCP, 0 },
	{ "lpd",          I_D | 0700,  DAEM, GDAEM, 0 },
	{ "mail",         I_D | 0755,  DAEM, GDAEM, 0 },
	{ NULL }
};

/* '/usr/lib' */
static FileInfo usrLibFiles[] = {
	{ "pwdauth",      I_R | 0755,  BIN,  GOP,   0, "commands/simple/pwdauth.bin" },
	{ NULL }
};

/* '/usr/adm' */
static FileInfo usrAdmFiles[] = {
	{ "wtmp",		  I_R | 0644,  ROOT, GOP,   0, "tools/res/usr/adm/wtmp" },
	{ "lastlog",	  I_R | 0644,  ROOT, GOP,   0, "tools/res/usr/adm/lastlog" },
	{ NULL }
};

/* '/usr' */
static FileInfo usrDirs[] = {
	{ "bin",          I_D | 0755,  BIN,  GOP,   0, NULL, NULL, { commonBinFiles, usrBinFiles, NULL } },
	{ "sbin",         I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { usrSbinFiles, NULL } },
	{ "adm",          I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { usrAdmFiles, NULL } },
	{ "run",          I_D | 0755,  ROOT, GOP,   0 },
	{ "log",          I_D | 0755,  ROOT, GOP,   0 },
	{ "spool",        I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { usrSpoolFiles, NULL } },
	{ "etc",          I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { usrEtcFiles, NULL } },
	{ "tmp",          I_D | 0777,  ROOT, GOP,   0 },
	{ "lib",          I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { usrLibFiles, NULL } },
	{ "preserve",     I_D | 0700,  ROOT, GOP,   0 },
	{ NULL }
};


/* '/', mounted '/usr', mounted '/home' */
static FileInfo rootInfo =
    { NULL,           I_D | 0755,  ROOT, GOP,   0, NULL, NULL, { rootDirs, NULL } };
static FileInfo homeInfo =
    { NULL,           I_D | 0777,  ROOT, GOP,   0 };
static FileInfo usrInfo =
    { NULL,           I_D | 0777,  ROOT, GOP,   0, NULL, NULL, { usrDirs, NULL } };


static FS fsList[] = {
	{ "ROOT", &rootInfo },
	{ "HOME", &homeInfo },
	{ "USR",  &usrInfo },
	{ NULL }
};

