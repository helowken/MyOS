#ifndef TYPE_H
#define TYPE_H

#include "sys/types.h"

typedef unsigned reg_t;	/* Machine register */

typedef struct {
	u16_t gs;
	u16_t fs;
	u16_t es;
	u16_t ds;
	reg_t di;			/* di through cx are not accessed in C */
	reg_t si;			/* Order is to match pusha/popa */
	reg_t bp;			
	reg_t temp;			
	reg_t bx;
	reg_t dx;
	reg_t cx;
	reg_t ax;			/* gs through ax are all pushed by save() in assembly */
	reg_t retAddr;		/* Return address for save() in assembly */
	reg_t ip;			/* ip, cs, eflags are pushed by interrupt */
	reg_t cs;
	reg_t eflags;
	reg_t sp;			/* sp, ss are pushed by processor when a stack swithed */
	reg_t ss;
} StackFrame;

typedef struct {
	u16_t limitLow;
	u16_t baseLow;
	u8_t baseMiddle;
	u8_t access;		/* | P |    DPL    | S=1 | Data(0|E|W|A) / Code(1|C|R|A) | */
	u8_t granularity;	/* | G | D/B | L=0 | AVL | LIMIT | */
	u8_t baseHigh;
} SegDesc;

#endif
