#include "timers.h"

typedef struct MProc MProc;
typedef struct Memory Memory;

/* alloc.c */
void initMemory(Memory *chunks, phys_clicks *free);
void freeMemory(phys_clicks base, phys_clicks clicks);
void swapInQueue(MProc *rmp);

/* utility.c */
void panic(char *who, char *msg, int num);
char *findParam(const char *key);
int getMemMap(int pNum, MemMap *memMap);
pid_t getFreePid();
int noSys();
void tellFS(int what, int p1, int p2, int p3);
int getStackPtr(int pNum, vir_bytes *sp);

/* break.c */
int doBrk();

/* timers.c */
void pmSetTimer(Timer *tp, int delta, TimerFunc watchdog, int arg);
void pmExpireTimers(clock_t now);
void pmCancelTimer(Timer *tp);

/* forkexit.c */
int doFork();
int doPmExit();
int doWaitPid();
void pmExit(MProc *rmp, int exitStatus);

/* exec.c */
int doExec();

/* trace.c */
int doTrace();
void stopProc(MProc *rmp, int sigNum);

/* misc.c */
int doReboot();
int doGetSysInfo();
int doGetProcNum();
int doSvrCtl();
int doAllocMem();
int doFreeMem();
int doGetSetPriority();
void setReply(int pIdx, int result);

/* table.c */
void initSysCalls();

/* getset.c */
int doGetSet();

/* signal.c */
int doAlarm();
int doKill();
int kernelSigPending();
int doPause();
int setAlarm(int pNum, int sec);
int checkSig(pid_t pid, int sigNum);
void signalProc(MProc *rmp, int sigNum);
int doSigAction();
int doSigPending();
int doSigProcMask();
int doSigReturn();
int doSigSuspend();
void checkPending(MProc *rmp);

/* time.c */
int doSTime();
int doTime();
int doTimes();
int doGetTimeOfDay();


