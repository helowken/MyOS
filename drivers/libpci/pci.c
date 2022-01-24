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

static u16_t pciReadStatus(int devInd) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	return pciBusList[busInd].pb_rd_status(busInd);
}

static void pciWriteStatus(int devInd, u16_t value) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	pciBusList[busInd].pb_wr_status(busInd, value);
}

u8_t pciAttrR8(int devInd, int port) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	return pciBusList[busInd].pb_rd_reg8(busInd, devInd, port);
}

u16_t pciAttrR16(int devInd, int port) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	return pciBusList[busInd].pb_rd_reg16(busInd, devInd, port);
}

u32_t pciAttrR32(int devInd, int port) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	return pciBusList[busInd].pb_rd_reg32(busInd, devInd, port);
}

void pciAttrW16(int devInd, int port, u16_t value) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	pciBusList[busInd].pb_wr_reg16(busInd, devInd, port, value);
}

void pciAttrW32(int devInd, int port, u32_t value) {
	int busInd;

	busInd = pciDevList[devInd].pd_bus_ind;
	pciBusList[busInd].pb_wr_reg32(busInd, devInd, port, value);
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
			
			pciWriteStatus(devInd, PSR_SSE | PSR_RMAS | PSR_RTAS);
			vid = pciAttrR16(devInd, PCI_VID);
			did = pciAttrR16(devInd, PCI_DID);
			headerType = pciAttrR8(devInd, PCI_HEADER_TYPE);
			status = pciReadStatus(devInd);
			
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

			baseClass = pciAttrR8(devInd, PCI_BASE_CLASS);
			subClass = pciAttrR8(devInd, PCI_SUB_CLASS);
			intfClass = pciAttrR8(devInd, PCI_PROG_IF);
			s = pciSubClassName(baseClass, subClass, intfClass);
			if (!s) {
				s = pciBaseClassName(baseClass);	
				if (!s)
				  s = "(unknown class)";
			}
			printf("\tclass %s (%X/%X/%X)\n", s, baseClass, subClass, intfClass);

		}
	}

	if (false)
	  printf("%s", headerType);
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
	int i, busInd;
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




