#include "sys/types.h"

#define PCI_CONF_ADDR		0xCF8
#define		PCI_CODE		0x80000000
#define		PCI_BUS_MASK	0xff0000
#define		PCI_BUS_SHIFT	16
#define		PCI_DEV_MASK	0xf800
#define		PCI_DEV_SHIFT	11
#define		PCI_FUNC_MASK	0x700
#define		PCI_FUNC_SHIFT	8
#define		PCI_REG_MASK	0xfc
#define		PCI_REG_SHIFT	2
#define PCI_CONF_DATA		0xCFC

#define PCI_SEL_REG(bus, dev, func, reg) \
	(PCI_CODE | \
		(((bus) << PCI_BUS_SHIFT) & PCI_BUS_MASK) | \
		(((dev) << PCI_DEV_SHIFT) & PCI_DEV_MASK) | \
		(((func) << PCI_FUNC_SHIFT) & PCI_FUNC_MASK) | \
		((((reg) / 4) << PCI_REG_SHIFT) & PCI_REG_MASK))		/* Naturally 4-byte aligned */
#define PCI_UNSEL			0

#define PCI_RD_REG8(bus, dev, func, reg) \
	(pciOutl(PCI_CONF_ADDR, PCI_SEL_REG(bus, dev, func, reg)), \
	 pciInb(PCI_CONF_DATA + ((reg) & 3)))
#define PCI_RD_REG16(bus, dev, func, reg) \
	(PCI_RD_REG8(bus, dev, func, reg) | \
	 PCI_RD_REG8(bus, dev, func, reg + 1) << 8)
#define PCI_RD_REG32(bus, dev, func, reg) \
	(PCI_RD_REG16(bus, dev, func, reg) | \
	 PCI_RD_REG16(bus, dev, func, reg + 2) << 16)

#define PCI_WR_REG8(bus, dev, func, reg, val) \
	(pciOutl(PCI_CONF_ADDR, PCI_SEL_REG(bus, dev, func, reg)), \
	 pciOutb(PCI_CONF_DATA + ((reg) & 3), (val)))
#define PCI_WR_REG16(bus, dev, func, reg, val) \
	(PCI_WR_REG8(bus, dev, func, reg, (val)), \
	 PCI_WR_REG8(bus, dev, func, reg + 1, (val) >> 8))
#define PCI_WR_REG32(bus, dev, func, reg, val) \
	(PCI_WR_REG16(bus, dev, func, reg, (val)), \
	 PCI_WR_REG16(bus, dev, func, reg, (val) >> 16))

/* PIIX configuration registers */
#define	PIIX_PIRQRCA	0x60
#define		PIIX_IRQ_DISABLE	0x80
#define		PIIX_IRQ_MASK		0x0F

/* PIIX extensions to the PIC */
#define PIIX_ELCR1	0x4D0
#define PIIX_ELCR2	0x4D1


