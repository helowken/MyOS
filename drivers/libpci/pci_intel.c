#include "sys/types.h"
#include "pci.h"
#include "pci_intel.h"  

u8_t PCI_RD_REG8(int bus, u8_t dev, u8_t func, int reg) {
	pciOutl(PCI_CONF_ADDR, PCI_SEL_REG(bus, dev, func, reg));
	return pciInb(PCI_CONF_DATA + (reg & 3));
}

u16_t PCI_RD_REG16(int bus, u8_t dev, u8_t func, int reg) {
	pciOutl(PCI_CONF_ADDR, PCI_SEL_REG(bus, dev, func, reg));
	return pciInw(PCI_CONF_DATA + (reg / 2 * 2));
}

u32_t PCI_RD_REG32(int bus, u8_t dev, u8_t func, int reg) {
	pciOutl(PCI_CONF_ADDR, PCI_SEL_REG(bus, dev, func, reg));
	return pciInl(PCI_CONF_DATA + (reg / 4));
}

void PCI_WR_REG8(int bus, u8_t dev, u8_t func, int reg, u8_t value) {
	pciOutl(PCI_CONF_ADDR, PCI_SEL_REG(bus, dev, func, reg)); 
	pciOutb(PCI_CONF_DATA + (reg & 3), value);
}

void PCI_WR_REG16(int bus, u8_t dev, u8_t func, int reg, u16_t value) {
	pciOutl(PCI_CONF_ADDR, PCI_SEL_REG(bus, dev, func, reg)); 
	pciOutw(PCI_CONF_DATA + (reg / 2 * 2), value);
}

void PCI_WR_REG32(int bus, u8_t dev, u8_t func, int reg, u32_t value) {
	pciOutl(PCI_CONF_ADDR, PCI_SEL_REG(bus, dev, func, reg)); 
	pciOutl(PCI_CONF_DATA + (reg / 4), value);
}

