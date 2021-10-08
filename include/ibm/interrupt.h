#ifndef _INTERRUPT_H
#define _INTERRUPT_H

/* 
 * Interrupt vectors defined/reserved by processor.
 *
 * Type: A(bort) / F(ault) / I(nterrupt) / T(rap) 
 * Error Code (EC): Y(es) / N(o)
 */
									/* Description      | Type | EC */
#define DIVIDE_VECTOR		0		/* Divide Error     |    F | N  */
#define DEBUG_VECTOR		1		/* Debug Exception  |    F | N  */ 
#define NMI_VECTOR          2		/* NMI Interrupt    |    I | N  */
#define BREAKPOINT_VECTOR	3		/* Breakpoint       |    T | N  */
#define OVERFLOW_VECTOR		4		/* Overflow         |    T | N  */

/*
 * Switchable irq bases for hardware interrupts.
 */
#define IRQ8_VECTOR			0X70	/* No need to move IRQ8-15 */

#endif
