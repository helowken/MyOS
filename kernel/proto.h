#ifndef PROTO_H
#define PROTO_H

/* protect.c */
phys_bytes seg2Phys(U16_t seg);
void protectInit();


/* start.c */
void cstart(U16_t cs, U16_t ds,	U16_t mds, U16_t paramOffset,U16_t paramSize);	


/* exception.c */
void handleException(unsigned vectorNum);


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

#endif
