#ifndef PROTO_H
#define PROTO_H

struct Proc;

/* protect.c */
phys_bytes seg2Phys(U16_t seg);
void protectInit();

/* clock.c */
void clockTask();

/* start.c */
void cstart(U16_t cs, U16_t ds,	U16_t mds, U16_t paramOffset,U16_t paramSize);	


/* exception.c */
void handleException(unsigned vectorNum);


/* i8259.c */
void initInterrupts();

/* system.c */
int getPriv(register struct Proc *rp, int procType);
void sysTask();

/* klib386.S */
void physCopy(phys_bytes source, phys_bytes dest, phys_bytes count);

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

#endif
