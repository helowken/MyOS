#include "../drivers.h"
#include "pci.h"

PciVendor pciVendorTable[] = {
	{ 0x8086, "Intel" },
	{ 0x0000, NULL } 
};

PciDevItem pciDeviceTable[] = {
	{ 0x8086, 0x1237, "Intel 82441FX (440FX)" },
	{ 0x8086, 0x7190, "Intel 82443BX" },
	{ 0x0000, 0x0000, NULL }
};

PciIntelCtrl pciIntelCtrlTable[] = {
	{ 0x8086, 0x1237 },		/* Intel 82441FX (i440FX in Bochs) */
	{ 0x8086, 0x7190 },		/* Intel 82443BX - AGP enabled (i440BX in Bochs) */
	{ 0x0000, 0x0000 }
};

PciBaseClass pciBaseClassTable[] = {
	{ 0x00, "No device class" },
	{ 0x01, "Mass Storage Controller" },
	{ 0x02, "Network Controller" },
	{ 0x03, "Display Controller" },
	{ 0x04, "Multimedia Controller" },
	{ 0x05, "Memory Controller" },
	{ 0x06, "Bridge" },
	{ 0x07, "Simple Communication Controller" },
	{ 0x08, "Base System Peripheral" },
	{ 0x09, "Input Device Controller" },
	{ 0x0A, "Docking Station" },
	{ 0x0B, "Processor" },
	{ 0x0C, "Serial Bus Controller" },
	{ 0x0D, "Wireless Controller" },
	{ 0x0E, "Intelligent Controller" },
	{ 0x0F, "Satellite Communication Controller" },
	{ 0x10, "Encryption Controller" },
	{ 0x11, "Signal Processing Controller" },
	{ 0x12, "Processing Accelerator" },
	{ 0x13, "Non-Essential Instrumentation" },
	{ 0x40, "Co-Processor" },
	{ 0xff, "Unassigned Class (Vendor specific)" },
	{ 0x00, NULL }
};

PciSubClass pciSubClassTable[] = {
	{ 0x00, 0x01, 0x00, "VGA-Compatible Unclassified Device" },
	
	{ 0x01, 0x00, 0x00, "SCSI Bus Controller" },
	{ 0x01, 0x01, -1,   "IDE Controller" },
	{ 0x01, 0x02, 0x00, "Floppy Disk Controller" },
	{ 0x01, 0x03, 0x00, "IPI Bus Controller" },
	{ 0x01, 0x04, 0x00, "IPI Bus Controller" },
	{ 0x01, 0x05, -1,   "ATA Controller" },
	{ 0x01, 0x06, -1,   "Serial ATA Controller" },
	{ 0x01, 0x07, -1,   "Serial Attached SCSI Controller" },
	{ 0x01, 0x08, -1,   "Non-Volatile Memory Controller" },
	{ 0x01, 0x80, 0x00, "Other Mass Storage Controller" },

	{ 0x02, 0x00, 0x00, "Ethernet Controller" },
	{ 0x02, 0x01, 0x00, "Token Ring Controller" },
	{ 0x02, 0x02, 0x00, "FDDI Controller" },
	{ 0x02, 0x03, 0x00, "ATM Controller" },
	{ 0x02, 0x04, 0x00, "ISDN Controller" },
	{ 0x02, 0x05, 0x00, "WorldFip Controller" },
	{ 0x02, 0x06, 0x00, "PICMG 2.14 Multi Computing Controller" },
	{ 0x02, 0x07, 0x00, "Infiniband Controller" },
	{ 0x02, 0x08, 0x00, "Fabric Controller" },
	{ 0x02, 0x80, 0x00, "Other Network Controller" },

	{ 0x03, 0x00, 0x00, "VGA Controller" },
	{ 0x03, 0x00, 0x01, "8514-Compatible Controller" },
	{ 0x03, 0x01, 0x00, "XGA Controller" },
	{ 0x03, 0x02, 0x00, "3D Controller (Not VGA-Compatible)" },
	{ 0x03, 0x80, 0x00, "Other Display Controller" },

	{ 0x04, 0x00, 0x00, "Multimedia Video Controller" },
	{ 0x04, 0x01, 0x00, "Multimedia Audio Controller" },
	{ 0x04, 0x02, 0x00, "Computer Telephony Device" },
	{ 0x03, 0x03, 0x00, "Audio Device" },
	{ 0x03, 0x80, 0x00, "Other Multimedia Controller" },

	{ 0x06, 0x00, 0x00, "Host Bridge" },
	{ 0x06, 0x01, 0x00, "ISA Bridge" },
	{ 0x06, 0x02, 0x00, "EISA Bridge" },
	{ 0x06, 0x03, 0x00, "MCA Bridge" },
	{ 0x06, 0x04, 0x00, "Normal Decode PCI-to-PCI Bridge" },
	{ 0x06, 0x04, 0x01, "Subtractive Decode PCI-to-PCI Bridge" },
	{ 0x06, 0x05, 0x00, "PCMCIA Bridge" },
	{ 0x06, 0x06, 0x00, "NuBus Bridge" },
	{ 0x06, 0x07, 0x00, "CardBus Bridge" },
	{ 0x06, 0x08, -1,   "RACEway Bridge" },
	{ 0x06, 0x09, -1,   "Semi-Transparent PCI-to-PCI Bridge" },
	{ 0x06, 0x0A, 0x00, "InfiniBand-to-PCI Host Bridge" },
	{ 0x06, 0x80, 0x00, "Other Bridge" },

	{ 0x0C, 0x00, 0x00, "IEEE 1394 FireWire Controller" },
	{ 0x0C, 0x00, 0x01, "IEEE 1394 OHCI Controller" },
	{ 0x0C, 0x01, 0x00, "ACCESS Bus Controller" },
	{ 0x0C, 0x02, 0x00, "SSA" },
	{ 0x0C, 0x03, 0x00, "USB (with UHCI)" },
	{ 0x0C, 0x03, 0x10, "USB (with OHCI)" },
	{ 0x0C, 0x03, 0x20, "USB2 (with EHCI)" },
	{ 0x0C, 0x03, 0x30, "USB3 (with XHCI)" },
	{ 0x0C, 0x03, 0x80, "USB (other host inf.)" },
	{ 0x0C, 0x03, 0xFE, "USB Device" },
	{ 0x0C, 0x04, 0x00, "Fibre Channel" },
	{ 0x0C, 0x05, 0x00, "SMBus Controller" },
	{ 0x0C, 0x06, 0x00, "InfiniBand Controller" },
	{ 0x0C, 0x07, -1,   "IPMI Interface" },
	{ 0x0C, 0x08, 0x00, "SERCOS Interface (IEC 61491)" },
	{ 0x0C, 0x09, 0x00, "CANbus Controller" },
	{ 0x0C, 0x80, 0x00, "Other Serial Bus Controller" },

	{ 0x00, 0x00, 0x00, NULL }
};

PciIsaBridge pciIsaBridgeTable[] = {
	{ 0x8086, 0x7000, 1, PCI_IB_PIIX },		/* Intel 82371SB */
	{ 0x0000, 0x0000, 0, 0 }
};

PciPciBridge pciPciBridgeTable[] = {
	{ 0x8086, 0x244e, PCI_PCIB_INTEL },		/* Intel 82801 PCI Bridge */
	{ 0x8086, 0x2561, PCI_AGPB_INTEL },		/* Intel 82845 AGP Bridge */
	{ 0x8086, 0x7191, PCI_AGPB_INTEL },		/* Intel 82443BX (AGP bridge) */ 
	{ 0x0000, 0x0000, 0 }
};
