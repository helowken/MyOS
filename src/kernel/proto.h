#ifndef PROTO_H
#define PROTO_H

typedef struct Proc Proc;

/* protect.c */
phys_bytes seg2Phys(u16_t seg);
void protectInit();
void allocSegments(Proc *rp);
void initDataSeg(register SegDesc *sdPtr, phys_bytes base, 
			vir_bytes size, int privilege);

/* clock.c */
void clockTask();
void clockStop();
clock_t getUptime();
void setTimer(struct Timer *tp, clock_t expTime, TimerFunc watchDog);
void resetTimer(struct Timer *tp);
unsigned long readClock();

/* main.c */
void main();
void prepareShutdown(int how);

/* utility.c */
void kprintf(const char *fmt, ...);
void panic(const char *s, int n);

/* proc.c */
void lockEnqueue(Proc *rp);
void lockDequeue(Proc *rp);
int sys_call(int function, int srcDst, Message *msg);

/* start.c */
void cstart(u16_t cs, u16_t ds,	u16_t mds, u16_t paramOffset,u16_t paramSize);	

/* exception.c */
void handleException(unsigned exVec, unsigned trapErrno, 
			unsigned long eip, unsigned cs, unsigned eflags);

/* i8259.c */
void initInterrupts();
void putIrqHandler(IrqHook *hook, int irq, IrqHandler handler);
void handleInterrupt(IrqHook *hook);
void removeIrqHandler(IrqHook *hook);
int lockSend(int dst, Message *msg);
int lockNotify(int src, int dst);

/* system.c */
int getPriv(register Proc *rp, int procType);
void sysTask();
#define nUMapLocal(pNum, virAddr, bytes) \
	umapLocal(procAddr(pNum), D, (virAddr), (bytes))
phys_bytes umapLocal(Proc *rp, int seg, vir_bytes virAddr, vir_bytes bytes);
phys_bytes umapRemote(Proc *rp, int seg, vir_bytes virAddr, vir_bytes bytes);
phys_bytes umapBios(Proc *rp, vir_bytes virAddr, vir_bytes bytes);
void sendSig(int pNum, int sig);
void causeSig(int pNum, int sig);
int virtualCopy(VirAddr *src, VirAddr *dst, vir_bytes bytes);
void getRandomness(int source);

/* klib386.S */
void copyMessage(int src, phys_clicks srcAddr, vir_bytes srcOffset, 
			phys_clicks dstAddr, vir_bytes dstOffset);
void physCopy(phys_bytes source, phys_bytes dest, phys_bytes count);
void physMemset(phys_bytes source, unsigned long pattern, phys_bytes count);
void enableIrq(IrqHook *hook);
void disableIrq(IrqHook *hook);
void level0(void (*func)());
void readTsc(unsigned long *high, unsigned long *low);
void physInsb(u16_t port, phys_bytes buf, size_t count);
void physInsw(u16_t port, phys_bytes buf, size_t count);
void physOutsb(u16_t port, phys_bytes buf, size_t count);
void physOutsw(u16_t port, phys_bytes buf, size_t count);
void monitor();
void reset();


/* mpx.S */
void idleTask();
void restart();


/* The following are never called from C (pure asm procs). */

/* Exception handlers (protected mode), in numerical order. */
void divideError();
void singleStepException();
void nmi();
void breakpointException();
void overflow();
void boundsCheck();
void invalOpcode();
void coprNotAvailable();
void doubleFault();
void coprSegOverrun();
void invalTss();
void segmentNotPresent();
void stackException();
void generalProtection();
void pageFault();
void coprError();

/* Hardware interrupt handlers. */
void hwint00();
void hwint01();
void hwint02();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint09();
void hwint10();
void hwint11();
void hwint12();
void hwint13();
void hwint14();
void hwint15();

/* Software interrupt handlers, in numerical order. */
void s_call();
void level0_call();

#endif
