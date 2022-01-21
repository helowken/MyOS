#include "../drivers.h"
#include "pci.h"

PciVendor pciVendorTable[] = {
	{ 0x8086, "Intel" },
	{ 0x0000, NULL } 
};

PciIntelCtrl pciIntelCtrlTable[] = {
	{ 0x8086, 0x1237 },		/* Intel 82441FX (i440FX in Bochs) */
	{ 0x8086, 0x7190 },		/* Intel 82443BX - AGP enabled (i440BX in Bochs) */
	{ 0x0000, 0x0000 }
};
