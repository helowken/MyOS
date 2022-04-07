#ifndef _INTERRUPT_H
#define _INTERRUPT_H

/* 8259A interrupt controller ports. */
#define INT_CTL			0x20	/* I/O port for interrupt controller (Master) */
#define INT_CTL_MASK	0x21	/* Setting bits in this port disables interrupts. (Master) */
#define INT2_CTL		0xA0	/* I/O port for the second interrupt controller (Slave) */
#define INT2_CTL_MASK	0xA1	/* Setting bits in this port disables interrupts. (Slave) */

/* Magic numbers for interrupt controller. */
#define END_OF_INT		0x20	/* Code used to re-enable after an interrupt. (See OCW2 in i8259.c) */

/* Fixed system call vector. */
#define SYS386_VECTOR	33		/* Except 386 system calls use this. */
#define LEVEL0_VECTOR	34		/* For execution of a function at level 0. */

/* 
 * Interrupt vectors defined/reserved by processor.
 * Type: A(bort) / F(ault) / I(nterrupt) / T(rap) 
 * Error Code (EC): Y(es) / N(o)
 */
								/* Description      | Type | EC */
#define DIVIDE_VECTOR		0	/* Divide Error     |    F | N  */
#define DEBUG_VECTOR		1	/* Debug Exception  |    F | N  */ 
#define NMI_VECTOR          2	/* NMI Interrupt    |    I | N  */
#define BREAKPOINT_VECTOR	3	/* Breakpoint       |    T | N  */
#define OVERFLOW_VECTOR		4	/* Overflow         |    T | N  */

/*
 * Switchable irq bases for hardware interrupts.
 * Reprogram the 8259(s) from the PC BIOS defaults since the BIOS 
 * doesn't respect all the processor's reserved vectors (0 to 31).
 */
#define BIOS_IRQ0_VEC	0x08	/* Base of IRQ0-7vectors used by BIOS */
#define BIOS_IRQ8_VEC	0x70	/* Base of IRQ8-15 vectors used by BIOS */
#define IRQ0_VECTOR		0x50	/* Nice vectors to relocate IRQ0-7 to */
#define IRQ8_VECTOR		0X70	/* No need to move IRQ8-15 */

/* Hardware interrupt numbers. */
#define NR_IRQ_VECTORS	   16
#define CLOCK_IRQ			0
#define KEYBOARD_IRQ		1
#define CASCADE_IRQ			2	/* Cascade enable for 2nd AT controller */
#define ETHER_IRQ			3	/* Default ethernet interrupt vector */
#define SECONDARY_IRQ		3	/* RS232 interrupt vector for port 2 */
#define RS232_IRQ			4	/* RS232 interrupt vector for port 1 */
#define XT_WINI_IRQ			5	/* XT winchester */
#define FLOPPY_IRQ			6	/* Floppy disk */
#define PRINTER_IRQ			7
#define AT_WINI_0_IRQ	   14	/* AT winchester controller 0 */
#define AT_WINI_1_IRQ	   15	/* AT winchester controller 1 */

/* Interrupt number to hardware vector. */
#define BIOS_VECTOR(irq)	(((irq) < 8 ? BIOS_IRQ0_VEC : BIOS_IRQ8_VEC) + ((irq) & 0x7))
#define VECTOR(irq)			(((irq) < 8 ? IRQ0_VECTOR : IRQ8_VECTOR) + ((irq) & 0x7))

#endif
