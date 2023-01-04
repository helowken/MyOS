struct Inode;

#include "timers.h"
#include "buf.h"
#include "super.h"
#include "inode.h"
#include "file.h"

/* device.c */
int devOpen(dev_t dev, int proc, int flags);
void devClose(dev_t dev);
int devIO(int op, dev_t dev, int proc, void *buf, 
			off_t pos, int bytes, int flags);
int noDev(int op, dev_t dev, int proc, int flags);
int genOpCl(int op, dev_t dev, int proc, int flags);
int ttyOpCl(int op, dev_t dev, int proc, int flags);
int cttyOpCl(int op, dev_t dev, int proc, int flags);
int cloneOpCl(int op, dev_t dev, int proc, int flags);
void genIO(int taskNum, Message *msg);
void cttyIO(int taskNum, Message *msg);
void devStatus(Message *msg);
int doIoctl(void);

/* utility.c */
void panic(char *who, char *msg, int num);
time_t clockTime(void);
int noSys(void);
int fetchName(char *path, int len, int flag);

/* write.c */
Buf *newBlock(Inode *ip, off_t position);
void zeroBlock(Buf *bp);
void clearZone(Inode *ip, off_t pos, int flag);
int doWrite(void);

/* dmap.c */
void buildDMap(void);

/* filedes.c */
int getFd(int start, mode_t bits, int *k, Filp **fpp);
Filp *getFilp(int fd);

/* stadir.c */
int doChdir(void);
int doFstat(void);
int doStat(void);

/* super.c */
bit_t allocBit(SuperBlock *sp, int map, bit_t origin);
int readSuper(SuperBlock *sp);
SuperBlock *getSuper(dev_t dev);
int getBlockSize(dev_t dev);
void freeBit(SuperBlock *sp, int map, bit_t bitReturned);

/* pipe.c */
void suspend(int task);
void revive(int proc, int bytes);

/* protect.c */
int doAccess(void);
int doUmask(void);
int checkReadOnly(Inode *ip);
int checkForbidden(Inode *ip, mode_t accessDesired);

/* read.c */
int doRead(void);
void readAhead(void);
Buf *doReadAhead(Inode *ip, block_t baseBlock, 
			off_t position, unsigned bytesAhead);
block_t readMap(Inode *ip, off_t pos);
zone_t readIndirZone(Buf *bp, int index);
int readWrite(int rwFlag);

/* cache.c */
zone_t allocZone(dev_t dev, zone_t z);
Buf *getBlock(dev_t dev, block_t blockNum, int onlySearch);
void putBlock(Buf *bp, int blockType);
void flushAll(dev_t dev);
void freeZone(dev_t dev, zone_t num);
void rwBlock(Buf *bp, int rwFlag);
void rwScattered(dev_t dev, Buf **bufQueue, int queueSize, int rwFlag);
void invalidate(dev_t dev);

/* misc.c */
int doSync(void);
int doFsync(void);
int doFcntl(void);
int doFork(void);
int doSet(void);
int doExec(void);

/* open.c */
int doOpen(void);
int doCreat(void);
int doClose(void);
int doMkdir(void);
int doLseek(void);

/* path.c */
Inode *lastDir(char *path, char string[NAME_MAX]);
Inode *advance(Inode *dirIp, char string[NAME_MAX]);
int searchDir(Inode *dirIp, char string[NAME_MAX], ino_t *iNum, int flag);
Inode *eatPath(char *path);

/* inode.c */
Inode *allocInode(dev_t dev, mode_t bits);
void rwInode(Inode *ip, int rwFlag);
void updateTimes(Inode *ip);
Inode *getInode(dev_t dev, ino_t num);
void dupInode(Inode *ip);
void putInode(Inode *ip);
void freeInode(dev_t dev, ino_t num);
void wipeInode(Inode *ip);

/* link.c */
int doUnlink(void);
int doRename(void);
void truncate(Inode *ip);

/* select.c */
void initSelect(void);
int selectNotified(int major, int minor, int ops);

/* timers.c */
void fsInitTimer(Timer *tp);
void fsExpireTimers(clock_t now);

/* main.c */
void reply(int whom, int result);

/* table.c */
void initSysCalls(void);
