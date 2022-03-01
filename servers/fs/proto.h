struct Inode;

#include "timers.h"
#include "buf.h"
#include "super.h"
#include "inode.h"

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

/* utility.c */
void panic(char *who, char *msg, int num);
time_t clockTime();

/* dmap.c */
void buildDMap();

/* super.c */
int readSuper(SuperBlock *sp);
SuperBlock *getSuper(dev_t dev);
int getBlockSize(dev_t dev);

/* pipe.c */
void suspend(int task);
void revive(int proc, int bytes);

/* read.c */
void readAhead();
Buf *doReadAhead(Inode *ip, block_t baseBlock, 
			off_t position, unsigned bytesAhead);
block_t readMap(Inode *ip, off_t pos);

/* cache.c */
Buf *getBlock(dev_t dev, block_t blockNum, int onlySearch);
void putBlock(Buf *bp, int blockType);
void flushAll(dev_t dev);
void rwBlock(Buf *bp, int rwFlag);
void rwScattered(dev_t dev, Buf **bufQueue, int queueSize, int rwFlag);
void invalidate(dev_t dev);

/* misc.c */
int doSync();
int doFsync();

/* inode.c */
void rwInode(Inode *ip, int rwFlag);
void updateTimes(Inode *ip);
Inode *getInode(dev_t dev, int num);
void dupInode(Inode *ip);

/* select.c */
void initSelect();

/* timers.c */
void fsInitTimer(Timer *tp);
void fsExpireTimers(clock_t now);

/* main.c */
void reply(int whom, int result);


