#define ROOT_INO	((Ino_t) 1)		/* Inode number of root dri. */

Off_t rawSuper(int *blockSize);
Ino_t rawLookup(Ino_t cwd, char *path);
Off_t rawVir2Abs(Off_t virBlockNum);
void rawStat(Ino_t ino, struct stat *st);
Ino_t rawReadDir(char *name);

