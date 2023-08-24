/* Users */
#define ROOT	SUPER_USER
#define DAEM	1	/* Daemon */
#define BIN		2
#define AST		8

/* Groups */
#define	GOP		0	/* opeartor */
#define GDAEM	1	/* Daemon */
#define GBIN	2	
#define OTHER	3
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

