#include "../drivers.h"
#include "minix/com.h"
#include "minix/syslib.h"

#include "pci.h"
#include "pci_intel.h"

#include "stdlib.h"
#include "stdio.h"

#include "string.h"
#include "minix/sysutil.h"

#define NR_PCI_BUS	4
#define NR_PCI_DEV	40

#define PBT_INTEL	1 
#define PBT_PCI_BRIDGE	2
 
typedef struct {
	int pb_type;
	int pb_isa_bridge_dev;
	int pb_isa_bridge_type;
	
	int pb_dev_ind;
	int pb_bus;

	u8_t (*pb_rd_reg8)(int busInd, int devInd, int port);
	u16_t (*pb_rd_reg16)(int busInd, int devInd, int port);
	u32_t (*pb_rd_reg32)(int busInd, int devInd, int port);
	void (*pb_wr_reg16)(int busInd, int devInd, int port, u16_t value);
	void (*pb_wr_reg32)(int busInd, int devInd, int port, u32_t value);
	u16_t (*pb_rd_status)(int busInd);
	void (*pb_wr_status)(int busInd, u16_t value);
} PciBus;
static PciBus pciBusList[NR_PCI_BUS];
static int numPciBus = 0;

typedef struct {
	u8_t pd_bus_ind;
	u8_t pd_dev;
	u8_t pd_func;
	u8_t pd_base_class;
	u8_t pd_sub_class;
	u8_t pd_intf_class;
	u16_t pd_vid;
	u16_t pd_did;
	u8_t pd_in_use;
} PciDev;
static PciDev pciDevList[NR_PCI_DEV];
static int numPciDev = 0;

void disablePci() {
	int s;
	if ((s = sysOutl(PCI_CONF_ADDR, PCI_UNSEL)) != OK)
	  printf("PCI: warning, sysOutl failed: %d\n", s);
}

static u8_t pci_rd_reg8(int busInd, int devInd, int port) {
	u8_t v;

	v = PCI_RD_REG8(pciBusList[busInd].pb_bus, 
				pciDevList[devInd].pd_dev, 
				pciDevList[devInd].pd_func, 
				port);
	disablePci();
	return v;
}

static u16_t pci_rd_reg16(int busInd, int devInd, int port) {
	u16_t v;

	v = PCI_RD_REG16(pciBusList[busInd].pb_bus, 
				pciDevList[devInd].pd_dev, 
				pciDevList[devInd].pd_func, 
				port);
	disablePci();
	return v;
}

static u32_t pci_rd_reg32(int busInd, int devInd, int port) {
	u32_t v;

	v = PCI_RD_REG32(pciBusList[busInd].pb_bus, 
				pciDevList[devInd].pd_dev, 
				pciDevList[devInd].pd_func, 
				port);
	disablePci();
	return v;
}

static void pci_wr_reg16(int busInd, int devInd, int port, u16_t value) {
	PCI_WR_REG16(pciBusList[busInd].pb_bus, 
				pciDevList[devInd].pd_dev, 
				pciDevList[devInd].pd_func, 
				port, value);
	disablePci();
}

static void pci_wr_reg32(int busInd, int devInd, int port, u32_t value) {
	PCI_WR_REG32(pciBusList[busInd].pb_bus, 
				pciDevList[devInd].pd_dev, 
				pciDevList[devInd].pd_func, 
				port, value);
	disablePci();
}

static u16_t pci_rd_status(int busInd) {
	u16_t v;

	v = PCI_RD_REG16(pciBusList[busInd].pb_bus, 0, 0, PCI_STATUS);
	disablePci();
	return v;
}

static void pci_wr_status(int busInd, u16_t value) {
	PCI_WR_REG16(pciBusList[busInd].pb_bus, 0, 0, PCI_STATUS, value);
	disablePci();
}

static char *pciVendorName(u16_t vid) {
	int i;

	for (i = 0; pciVendorTable[i].name; ++i) {
		if (pciVendorTable[i].vid == vid)
		  return pciVendorTable[i].name;
	}
	return "unknown";
}

static char *pciDevName(u16_t vid, u16_t did) {
	int i;

	for (i = 0; pciDeviceTable[i].name; ++i) {
		if (pciDeviceTable[i].vid == vid &&
			pciDeviceTable[i].did == did)
		  return pciDeviceTable[i].name;
	}
	return NULL;
}

u8_t pciAttrDevR8(int devInd, int port) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	return pciBusList[busInd].pb_rd_reg8(busInd, devInd, port);
}

u16_t pciAttrDevR16(int devInd, int port) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	return pciBusList[busInd].pb_rd_reg16(busInd, devInd, port);
}

u32_t pciAttrDevR32(int devInd, int port) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	return pciBusList[busInd].pb_rd_reg32(busInd, devInd, port);
}

void pciAttrDevW16(int devInd, int port, u16_t value) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	pciBusList[busInd].pb_wr_reg16(busInd, devInd, port, value);
}

void pciAttrDevW32(int devInd, int port, u32_t value) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	pciBusList[busInd].pb_wr_reg32(busInd, devInd, port, value);
}

static u16_t pciReadDevStatus(int devInd) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	return pciBusList[busInd].pb_rd_status(busInd);
}

static void pciWriteDevStatus(int devInd, u16_t value) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	pciBusList[busInd].pb_wr_status(busInd, value);
}

static u16_t pciReadBridgeStatus(int busInd) {
	int devInd;

	devInd = pciBusList[busInd].pb_dev_ind;
	return pciAttrDevR16(devInd, PPB_SEC_STATUS);
}

static void pciWriteBridgeStatus(int busInd, u16_t value) {
	int devInd;

	devInd = pciBusList[busInd].pb_dev_ind;
	pciAttrDevW16(devInd, PPB_SEC_STATUS, value);
}

static char *pciSubClassName(u8_t baseClass, u8_t subClass, u8_t intfClass) {
	int i;

	for (i = 0; pciSubClassTable[i].name; ++i) {
		if (pciSubClassTable[i].baseClass != baseClass || 
			pciSubClassTable[i].subClass != subClass)
		  continue;
		if (pciSubClassTable[i].intfClass != intfClass && 
			pciSubClassTable[i].intfClass != -1)
		  continue;
		return pciSubClassTable[i].name;
	}
	return NULL;
}

static char *pciBaseClassName(u8_t baseClass) {
	int i;

	for (i = 0; pciBaseClassTable[i].name; ++i) {
		if (pciBaseClassTable[i].baseClass == baseClass) 
		  return pciBaseClassTable[i].name;
	}
	return NULL;
}

static void probeBus(int busInd) {
	u8_t dev, func;
	u16_t vid, did, status;
	u8_t headerType;
	u8_t baseClass, subClass, intfClass;
	int devInd;
	char *devStr, *s;

	if (numPciDev >= NR_PCI_DEV) 
	  panic("PCI", "too many PCI devices", numPciDev);
	devInd = numPciDev;

	for (dev = 0; dev < 32; ++dev) {
		for (func = 0; func < 8; ++func) {
			pciDevList[devInd].pd_bus_ind = busInd;
			pciDevList[devInd].pd_dev = dev;
			pciDevList[devInd].pd_func = func;
			
			pciWriteDevStatus(devInd, PSR_SSE | PSR_RMAS | PSR_RTAS);
			vid = pciAttrDevR16(devInd, PCI_VID);
			did = pciAttrDevR16(devInd, PCI_DID);
			headerType = pciAttrDevR8(devInd, PCI_HEADER_TYPE);
			status = pciReadDevStatus(devInd);
			
			if (vid == NO_VID)
			  break;	/* Nothing here */
			if (status & (PSR_SSE | PSR_RMAS | PSR_RTAS)) 
			  break;

			devStr = pciDevName(vid, did);
			if (devStr) {
				printf("%d.%u.%u: %s (%04X/%04X)\n",
					busInd, dev, func, devStr, vid, did);
			} else {
				printf("%d.%u.%u: Unknown device, vendor %04X (%s), device %04X\n",
					busInd, dev, func, vid, pciVendorName(vid), did);
			}

			baseClass = pciAttrDevR8(devInd, PCI_BASE_CLASS);
			subClass = pciAttrDevR8(devInd, PCI_SUB_CLASS);
			intfClass = pciAttrDevR8(devInd, PCI_PROG_IF);
			s = pciSubClassName(baseClass, subClass, intfClass);
			if (!s) {
				s = pciBaseClassName(baseClass);	
				if (!s)
				  s = "(unknown class)";
			}
			printf("\tclass %s (%X/%X/%X)\n", s, baseClass, subClass, intfClass);

			pciDevList[devInd].pd_base_class = baseClass;
			pciDevList[devInd].pd_sub_class = subClass;
			pciDevList[devInd].pd_intf_class = intfClass;
			pciDevList[devInd].pd_vid = vid;
			pciDevList[devInd].pd_did = did;
			pciDevList[devInd].pd_in_use = 0;

			if (++numPciDev >= NR_PCI_DEV)
			  panic("PCI", "too many PCI devices", numPciDev);
			devInd = numPciDev;

			if (func == 0 && !(headerType & PHT_MULTI_FUNC))
			  break;
		}
	}
}

static int doPiix(int devInd) {
	int s, i, irq, irqRc;
	u16_t elcr1, elcr2, elcr;	/* Edge/Level Control Registers */

	if ((s = sysInb(PIIX_ELCR1, &elcr1)) != OK)
	  printf("Warning, sysInb failed: %d\n", s);
	if ((s = sysInb(PIIX_ELCR2, &elcr2)) != OK)
	  printf("Warning, sysInb failed: %d\n", s);
	elcr = elcr1 | (elcr2 << 8);

	for (i = 0; i < 4; ++i) {
		irqRc = pciAttrDevR8(devInd, PIIX_PIRQRCA + i);
		if (irqRc & PIIX_IRQ_DISABLE) {
			printf("INT%c: disabled\n", 'A' + i);
		} else {
			irq = irqRc & PIIX_IRQ_MASK;
			printf("INT%c: %d\n", 'A' + i, irq);

			if (!(elcr & (1 << irq))) {
				printf("(warning) IRQ %d is not level triggered\n", irq);
			}
			// irqModePci(irq);
		}
	}
	return 0;
}

static int doIsaBridge(int busInd) {
	int unknownBridge = -1;
	int bridgeDev = -1;
	int i, j, r, type;
	u16_t vid, did;
	char *devStr;

	for (i = 0; i < numPciDev; ++i) {
		if (pciDevList[i].pd_bus_ind != busInd)
		  continue;
		if (pciDevList[i].pd_base_class == 0x06 &&
			pciDevList[i].pd_sub_class == 0x01 &&
			pciDevList[i].pd_intf_class == 0x00) {
			/* ISA bridge. Report if no supported bridge is found. */
			unknownBridge = i;
		}

		vid = pciDevList[i].pd_vid;
		did = pciDevList[i].pd_did;
		for (j = 0; pciIsaBridgeTable[j].vid != 0; ++j) {
			if (pciIsaBridgeTable[j].vid != vid ||
				pciIsaBridgeTable[j].did != did)
			  continue;
			if (pciIsaBridgeTable[j].checkClass && unknownBridge != i) {
				/* vid and did match, but class(base, sub, intf) not match, means
				 * it is a multi-function device, but not a bridge.
				 */
				continue;
			}
			break;
		}
		if (pciIsaBridgeTable[j].vid) {
			bridgeDev = i;
			break;
		}
	}

	if (bridgeDev != -1) {
		devStr = pciDevName(vid, did);
		if (!devStr)
		  devStr = "unknown device";
		printf("Found ISA bridge (%04X/%04X) %s\n", vid, did, devStr);
		
		pciBusList[busInd].pb_isa_bridge_dev = bridgeDev;
		type = pciIsaBridgeTable[j].type;
		pciBusList[busInd].pb_isa_bridge_type = type;

		switch (type) {
			case PCI_IB_PIIX:
				r = doPiix(bridgeDev);
				break;
			case PCI_IB_VIA:
			case PCI_IB_AMD:
			case PCI_IB_SIS:
				printf("Not support here, please see minix source code.");
				r = -1;
				break;
			default:
				panic("PCI", "unknown ISA bridge type", type);
		}
		return r;
	}

	if (unknownBridge == -1) {
		printf("(warning) no ISA bridge found on bus %d", busInd);
		return 0;
	}
	printf("(warning) unsupported ISA bridge %04X/%04X for bus %d\n",
			pciDevList[unknownBridge].pd_vid,
			pciDevList[unknownBridge].pd_did,
			busInd);
	return 0;
}

static void doPciBridge(int busInd) {
	int devInd, i, type, ind;
	u16_t vid, did;
	u8_t baseClass, subClass, intfClass;
	u8_t subBusNum;

	for (devInd = 0; devInd < numPciDev; ++devInd) {
		if (pciDevList[devInd].pd_bus_ind != busInd)
		  continue;
		
		vid = pciDevList[devInd].pd_vid;
		did = pciDevList[devInd].pd_did;
		for (i = 0; pciPciBridgeTable[i].vid != 0; ++i) {
			if (pciPciBridgeTable[i].vid != vid ||
				pciPciBridgeTable[i].did != did)
			  continue;
			break;
		}
		if (pciPciBridgeTable[i].vid == 0) {
			baseClass = pciAttrDevR8(devInd, PCI_BASE_CLASS);
			subClass = pciAttrDevR8(devInd, PCI_SUB_CLASS);
			intfClass = pciAttrDevR8(devInd, PCI_PROG_IF);
			if (baseClass != 0x06 || 
				subClass != 0x04 ||
				(intfClass != 0x00 && intfClass != 0x01)) {
				/* No a PCI-to-PCI bridge */
				continue;
			}
			printf("Ignoring unknown PCI-to-PCI bridge: %04X/%04X\n",
						vid, did);
			continue;
		}

		type = pciPciBridgeTable[i].type;
		printf("PCI-to-PCI bridge: %04X/%04X\n", vid, did);

		/* Assume that the BIOS initialized the secondary bus number. */
		subBusNum = pciAttrDevR8(devInd, PPB_SUB_BUS_NR);
		printf("Sub bus num = %d\n", subBusNum);

		if (numPciBus >= NR_PCI_BUS)
		  panic("PCI", "too many PCI buses", numPciBus);
		
		ind = numPciBus++;
		pciBusList[ind].pb_type = PBT_PCI_BRIDGE;
		pciBusList[ind].pb_isa_bridge_dev = -1;
		pciBusList[ind].pb_isa_bridge_type = 0;
		pciBusList[ind].pb_dev_ind = devInd;
		pciBusList[ind].pb_bus = subBusNum;
		pciBusList[ind].pb_rd_reg8 = pciBusList[busInd].pb_rd_reg8;
		pciBusList[ind].pb_rd_reg16 = pciBusList[busInd].pb_rd_reg16;
		pciBusList[ind].pb_rd_reg32 = pciBusList[busInd].pb_rd_reg32;
		pciBusList[ind].pb_wr_reg16 = pciBusList[busInd].pb_wr_reg16;
		pciBusList[ind].pb_wr_reg32 = pciBusList[busInd].pb_wr_reg32;

		switch (type) {
			case PCI_PCIB_INTEL:
			case PCI_AGPB_INTEL:
				pciBusList[ind].pb_rd_status = pciReadBridgeStatus;
				pciBusList[ind].pb_wr_status = pciWriteBridgeStatus;
				break;
			default:
				panic("PCI", "unknown PCI-PCI bridge type", type);
		}

		probeBus(ind);
	}
}

static void initIntelPci() {
/* Try to detect a know PCI controller. Read the Vendor ID and
 * the Device ID for function 0 of device 0.
 * Two times the value 0xffff suggests a system without a (compatible)
 * PCI controller. Only controllers with values listed in the table
 * pciIntelCtrl are actually used.
 */
	u32_t bus, dev, func;
	u16_t vid, did;
	int i, busInd, r;
	char *devStr;

	bus = dev = func = 0;

	vid = PCI_RD_REG16(bus, dev, func, PCI_VID);
	did = PCI_RD_REG16(bus, dev, func, PCI_DID);
	disablePci();

	if (vid == NO_VID && did == NO_DID)
	  return;	/* Nothing here */

	for (i = 0; pciIntelCtrlTable[i].vid; ++i) {
		if (pciIntelCtrlTable[i].vid == vid &&
			pciIntelCtrlTable[i].did == did)
		  break;
	}

	if (! pciIntelCtrlTable[i].vid)
	  printf("initIntelPci (warning): unknown PCI-controller:\n"
			"\tvendor %04X (%s), device %04X\n",
			vid, pciVendorName(vid), did);

	if (numPciBus >= NR_PCI_BUS)
	  panic("PCI", "too many PCI busses", numPciBus);

	busInd = numPciBus++;
	pciBusList[busInd].pb_type = PBT_INTEL;
	pciBusList[busInd].pb_isa_bridge_dev = -1;
	pciBusList[busInd].pb_isa_bridge_type = 0;
	pciBusList[busInd].pb_dev_ind = -1;
	pciBusList[busInd].pb_bus = 0;
	pciBusList[busInd].pb_rd_reg8 = pci_rd_reg8;
	pciBusList[busInd].pb_rd_reg16 = pci_rd_reg16;
	pciBusList[busInd].pb_rd_reg32 = pci_rd_reg32;
	pciBusList[busInd].pb_wr_reg16 = pci_wr_reg16;
	pciBusList[busInd].pb_wr_reg32 = pci_wr_reg32;
	pciBusList[busInd].pb_rd_status = pci_rd_status;
	pciBusList[busInd].pb_wr_status = pci_wr_status;

	devStr = pciDevName(vid, did);
	if (!devStr)
	  devStr = "unknown device";
	printf("initIntelPci: %s (%04X/%04X)\n", devStr, vid, did);

	probeBus(busInd);

	r = doIsaBridge(busInd);
	if (r != OK) {
		/* Disable all devices for this bus */
		for (i = 0; i < numPciDev; ++i) {
			if (pciDevList[i].pd_bus_ind == busInd)
			  pciDevList[i].pd_in_use = 1;
		}
		return;
	}

	/* Look for PCI bridge (for AGP) */
	doPciBridge(busInd);
}

void initPci() {
	static int firstTime = 1;

	if (!firstTime) 
	  return;

	// TODO env_parse
	
	firstTime = -1;
	/* Only Intel (compatible) PCI controllers are supported at the moment. */
	initIntelPci();
	firstTime = 0;
}

/*=====================================================================*
 *                helper functions for I/O                *
 *=====================================================================*/ 
unsigned pciInb(u16_t port) {
	u8_t value;
	int s;
	if ((s = sysInb(port, &value)) != OK)
	  printf("PCI: warning, sysInb failed: %d\n", s);
	return value;
}

unsigned pciInw(u16_t port) {
	u16_t value;
	int s;
	if ((s = sysInw(port, &value)) != OK)
	  printf("PCI: warning, sysInw failed: %d\n", s);
	return value;
}

unsigned pciInl(u16_t port) {
	u32_t value;
	int s;
	if ((s = sysInl(port, &value)) != OK)
	  printf("PCI: warning, sysInl failed: %d\n", s);
	return value;
}

void pciOutb(u16_t port, u8_t value) {
	int s;
	if ((s = sysOutb(port, value)) != OK)
	  printf("PCI: warning, sysOutb failed: %d\n", s);
}

void pciOutw(u16_t port, u16_t value) {
	int s;
	if ((s = sysOutw(port, value)) != OK)
	  printf("PCI: warning, sysOutw failed: %d\n", s);
}

void pciOutl(u16_t port, u32_t value) {
	int s;
	if ((s = sysOutl(port, value)) != OK)
	  printf("PCI: warning, sysOutl failed: %d\n", s);
}

static int pciAvailDev(int start, int *devIndPtr, u16_t *vidPtr, u16_t *didPtr) {
	int devInd;

	for (devInd = start; devInd < numPciDev; ++devInd) {
		if (!pciDevList[devInd].pd_in_use) {
			*devIndPtr = devInd;
			*vidPtr = pciDevList[devInd].pd_vid;
			*didPtr = pciDevList[devInd].pd_did;
			return 1;
		}
	}
	return 0;
}

int pciFirstAvailDev(int *devIndPtr, u16_t *vidPtr, u16_t *didPtr) {
	return pciAvailDev(0, devIndPtr, vidPtr, didPtr);
}


int pciNextAvailDev(int *devIndPtr, u16_t *vidPtr, u16_t *didPtr) {
	return pciAvailDev(*devIndPtr + 1, devIndPtr, vidPtr, didPtr);
}




