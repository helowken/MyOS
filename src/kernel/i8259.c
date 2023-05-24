/*
 * Each 8259A PIC can support eight IRQ lines.
 * It is very common to cascade a master PIC with up to eight slave PICs to support 64 IRQ lines.
 *
 * Two 8259A PIC chips can be used to support 15 interrupt sources.
 * Why 15 and not 16? That's because when you cascade chips, the PIC needs to use one of the 
 * interrupt lines to signal the other chip.
 *
 * Thus, in an AT, IRQ line 2 is used to signal the second chip. Becuase of this, IRQ 2 is not 
 * available for use by hardware devices, which got wired to IRQ 9 on the slave PIC instead.
 * The real mode BIOS used to set up an interrupt handler for IRQ 9 that redirects to the IRQ 2 
 * handler.
 */

/*
 *		Real Mode
 *
 *    Chip	  | Interrupt numbers (IRQ) | Vector offset | Interrupt Numbers
 * Master PIC | 0 to 7					| 0x08			| 0x08 to 0x0F
 * Slave  PIC | 8 to 15					| 0x70			| 0x70 to 0x77
 *
 * These default BIOS values suit real mode programming quite well; thy do not conflit with any
 * CPU exceptions like they do in protected mode.
 * Each interrupt vector has 4 bytes, containing 2 bytes for the IP register followed by 2 bytes
 * for the CS register. The 4 bytes together, CS:IP, form an address, pointing to the location
 * of an ISR.
 *
 *
 *		Protected Mode
 *
 * In protected mode, the IRQs 0 to 7 conflit with the CPU exception which are reserved by Intel 
 * up until 0x1F. Consequently it is difficult to tell the difference between an IRQ or an 
 * software error. It is thus recommended to change the PIC's offsets (also known as remapping
 * the PIC) so that IRQs use non-reserved vectors.
 */

/*
 *		Chip-Purpose		| I/O port
 *	Master PIC - Command	|	0x20
 *	Master PIC - Data		|	0x21
 *	Slave  PIC - Command	|	0xA0
 *	Slave  PIC - Data		|	0xA1
 */

/*
 *		Registers
 *
 *	IRR: interrupt request register, specify which interrupts are pending acknowledgment.
 *	ISR: in service register, is used to indicate which interrupts have already been acknowledged and
 *		 are being serviced by the processor.
 *	IRR and ISR are not directly accessible.
 *
 *	ICR: interrupt command/status register, consists of two registers (command register and 
 *		 status register) which share the same port.
 *		 The command register is write only, while the status register is read only.
 *		 PIC determines what register to access depending on whether the write or read line 
 *		 is asserted.
 *		 The command register is used to 
 *		 (a) initialize the PIC and
 *		 (b) clear the in-service register upon receipt of an EOI command.
 *	IMR: interrupt mask/data register, consists of two registers (mask register and data register)
 *		 which share the same port.
 *		 The mask register is obviously used to disable/enable interrupt sources.
 *		 The data register is used in the initialization phase to 
 *		 (a) configure PIC cascading and
 *		 (b) set up a base interrupt vector number for IRQ0.
 *	ICR and IMR are accessible to the processor. Their ports are shown below.
 *		Master | Slave
 *	ICR  0x20  | 0xA0
 *	IMR  0x21  | 0xA1
 */

/*
 * ICW Format (ICW1 ~ ICW4): initialization command word
 *
 * ICW1 (port=0x20/0xA0): start initialization
 *	D7:	0
 *	D6: 0
 *	D5:	0
 *	D4: 1 (indicate ICW1)
 *	D3: 1 (LTIM: 1 = level triggered mode, 0 = edge triggered mode)
 *	D2: 0 
 *	D1: 0 (SNGL: 1 = single, 0 = cascade mode)
 *	D0: 1 (1 = ICW4 needed, 0 = no ICW4 needed)
 *
 *
 * ICW2 (port=0x21): set up interrupt vector number
 *	D7: T7 (D7 ~ D3: interrupt vector address)
 *	D6: T6
 *	D5: T5
 *	D4: T4
 *	D3: T3
 *	D2: 0  (D2 ~ D0: IRQ vector)
 *	D1: 0
 *	D0: 0
 *
 *
 * ICW3 (master, port=0x21): set up which slaves are connected
 *	D7: 0 (D7 ~ D0: is connected to a slave)
 *	D6: 0
 *	D5: 0
 *	D4: 0
 *	D3: 0
 *	D2: 1 (a slave is connected)
 *	D1: 0
 *	D0: 0
 *
 *
 * ICW3 (slave, port=0xA1): set up slave id
 *	D7: 0 (D7 ~ D3: reserved)
 *	D6: 0
 *	D5: 0
 *	D4: 0
 *	D3: 0
 *	D2: 0 (D2 ~ D0: slave id)
 *	D1: 1 (corresponds to ICW3 master)
 *	D0: 0
 *
 *
 * ICW4 (port=0x20/0xA0):
 *  D7: 0 (D7 ~ D5: reserved)
 *  D6: 0
 *  D5: 0
 *  D4: 0 (SFNM: 1 = special fully nested mode, 0 = not special fully nested mode)
 *  D3: 0 (BUF: 1 = buffered mode, 0 = non buffered mode)
 *  D2: 0 (M/S, if BUF = 1, 1 = master, 0 = slave)
 *  D1: 0 (1 = auto EOI(end of interrupt), 0 = normal EOI)
 *  D0: 1 (1 = 8086/8088 mode)
 */

/*
 * OCW Format (OCW1 ~ OCW3): operation command word
 *
 * OCW1:
 *	D7: 1 (D7 ~ D0, interrupt mask)
 *	D6: 1
 *	D5: 1
 *	D4: 1
 *	D3: 1
 *	D2: 0
 *	D1: 1
 *	D0: 1
 *
 *
 * OCW2: 
 *	D7:	0	
 *	D6: 0 
 *	D5: 1
 *	  D7 ~ D5:
 *	  0 0 1: non-specific EOI command
 *	  0 1 1: specific EOI command
 *	  1 0 1: rotate on non-specific EOI command
 *	  1 0 0: rotate in automatic EOI mode (set)
 *	  0 0 0: rotate in automatic EOI mode (clear)
 *	  1 1 1: rotate on specific EOI command
 *	  1 1 0: set priority command
 *	  0 1 0: no operation
 *	D4: 0 (D4 = 0 and D3 = 0 indicates OCW2)
 *	D3: 0
 *	D2: 0 (when D6 = 1, D2 ~ D0 specify a IRQ, otherwise for all IRQs)
 *	D1: 0
 *	D0: 0
 * Note: if the IRQ came from the master PIC, it is sufficient to issue EOI only 
 * to the master PIC; however if the IRQ came from the slave PIC, it is necessary 
 * to issue EOI to both PIC chips.
 *
 *
 * OCW3:
 *	D7: 0
 *	D6: 0 
 *	D5: 0 
 *	  D6 ~ D5:
 *	  0 0: reserved
 *	  0 1: reserved
 *	  1 1: set special mask
 *	D4: 0 (D4 = 0 and D3 = 1 indicates OCW3)
 *	D3: 1
 *	D2: (1 = poll command, 0 = no poll command)
 *	D1: 0
 *	D0: 0
 *	  D1 ~ D0: (read register command)
 *	  0 0: no action
 *	  0 1: no action
 *	  1 0: read IRR next pulse
 *	  1 1: read ISR next pulse
 */

#include "kernel.h"
#include "proc.h"
#include <stddef.h>
#include <ibm/portio.h>
#include <minix/com.h>

#define ICW1_AT			0x11	/* edge triggered, cascade, need ICW4 */
#define ICW1_PC			0x13	/* edge triggered, no cascade, need ICW4 */
#define ICW1_PS			0x19	/* level triggered, cascade, need ICW4 */
#define ICW4_AT_SLAVE	0x01	/* not SFNM, not buffered, normal EOI, 8086 */
#define ICW4_AT_MASTER	0x05	/* not SFNM, not buffered, normal EOI, 8086 */
#define ICW4_PC_SLAVE	0x09	/* not SFNM, buffered, normal EOI, 8086 */
#define ICW4_PC_MASTER	0x0D	/* not SFNM, buffered, normal EOI, 8086 */


void initInterrupts(int mine) {
/* Initialize the 8259s, finishing with all interrupts disabled. This is
 * only done in protected mode, in real mode we don't touch the 8259s, but
 * use the BIOS locations instead. The flag "mine" is set if the 8259s are
 * to be programmed for MINIX, or to be reset to what the BIOS expects.
 */

	disableInterrupt();

	/* The AT and newer PS/2 have two interrupt controllers, one master, one slaved at IRQ2. */

	/* Settings for master. */
	outb(INT_CTL, machine.ps_mca ? ICW1_PS : ICW1_AT);
	outb(INT_CTL_MASK, mine ? IRQ0_VECTOR : BIOS_IRQ0_VEC);		/* ICW2 for master, set up interrupt vector number for IRQ0. */
	outb(INT_CTL_MASK, (1 << CASCADE_IRQ));		/* Use these lines to tell slaves. (only 1 slave here) */
	outb(INT_CTL_MASK, ICW4_AT_MASTER);
	/* IRQ 0-7 mask. Masking IRQ2 will cause the Slave PIC to stop raising IRQs. */
	outb(INT_CTL_MASK, ~(1 << CASCADE_IRQ));

	/* Settings for slave. */
	outb(INT2_CTL, machine.ps_mca ? ICW1_PS : ICW1_AT);
	outb(INT2_CTL_MASK, mine ? IRQ8_VECTOR : BIOS_IRQ8_VEC);	/* ICW2 for slave, set up interrupt vector number for IRQ8. */
	outb(INT2_CTL_MASK, CASCADE_IRQ);			/* Set up slave id. */
	outb(INT2_CTL_MASK, ICW4_AT_MASTER);
	outb(INT2_CTL_MASK, ~0);					/* IRQ 8-15 mask */
	
	/* Copy the BIOS vectors from the BIOS to the Minix location, so we can still make BIOS 
	 * calls without reprogramming the i8259s. (See BIOS interrupt vectors aboved.)
	 */
	physCopy(BIOS_VECTOR(0) * 4L, VECTOR(0) * 4L, 8 * 4L);		/* IRQ 0-7 */
	physCopy(BIOS_VECTOR(8) * 4L, VECTOR(8) * 4L, 8 * 4L);		/* IRQ 8-15 */
}

/* Register an interrupt handler. */
void putIrqHandler(IrqHook *hook, int irq, IrqHandler handler) {
	int id;
	IrqHook **line;

	if (irq < 0 || irq >= NR_IRQ_VECTORS)
	  panic("Invalid call to putIrqHandler", irq);

	line = &irqHandlers[irq];
	id = 1;
	while (*line != NULL) {
		if (hook == *line) 
		  return;		/* Extra initialization, no need. */
		line = &(*line)->next;
		id <<= 1;
	}
	if (id == 0)
	  panic("Too many handlers for irq", irq);

	hook->next = NULL;
	hook->handler = handler;
	hook->irq = irq;
	hook->id = id;
	*line = hook;

	irqInUse |= 1 << irq;
}

/* Call the interrupt handlers for an interrupt with the given hook list.
 * The assembly part of the handler has already masked the IRQ, reenabled the
 * controller(s) and enabled interrupts.
 */
void handleInterrupt(IrqHook *hook) {
	/* Call list of handlers for an IRQ. */
	while (hook != NULL) {
		/* For each handler in the list, mark it active by setting its ID bit,
		 * call the function, and unmark it if the function returns true.
		 */
		irqActiveIds[hook->irq] |= hook->id;
		if ((*hook->handler)(hook))
		  irqActiveIds[hook->irq] &= ~hook->id;
		hook = hook->next;
	}

	/* The assembly code will now disable interrupts, unmask the IRQ if and only
	 * if all active ID bits are cleared, and restart a process.
	 */
}

/* Unregister an interrupt handler. */
void removeIrqHandler(IrqHook *hook) {
	int irq = hook->irq;
	int id = hook->id;
	IrqHook **line;

	if (irq < 0 || irq >= NR_IRQ_VECTORS) 
		panic("Invalid call to removeIrqHandler", irq);

	line = &irqHandlers[irq];
	while (*line != NULL) {
		if ((*line)->id == id) {
			*line = (*line)->next;
			if (! irqHandlers[irq])
			  irqInUse &= ~(1 << irq);
			return;
		}
		line = &(*line)->next;
	}
}


