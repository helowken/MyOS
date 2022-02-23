#include "timers.h"

/* device.c */
int devOpen(dev_t dev, int proc, int flags);
int devIO(int op, dev_t dev, int proc, void *buf, 
			off_t pos, int bytes, int flags);
int noDev(int op, dev_t dev, int proc, int flags);
int genOpCl(int op, dev_t dev, int proc, int flags);
int ttyOpCl(int op, dev_t dev, int proc, int flags);
int cttyOpCl(int op, dev_t dev, int proc, int flags);
int cloneOpCl(int op, dev_t dev, int proc, int flags);
void genIO(int taskNum, Message *msg);
void cttyIO(int taskNum, Message *msg);

/* utility.c */
void panic(char *who, char *msg, int num);

/* dmap.c */
void buildDMap();

/* super.c */
int readSuper(SuperBlock *sp);

/* pipe.c */
void suspend(int task);
void revive(int proc, int bytes);
