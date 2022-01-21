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
static PciBus pciBus[NR_PCI_BUS];
static int numPciBus = 0;

typedef struct {
	u8_t pd_busInd;
	u8_t pd_dev;
	u8_t pd_func;
	u8_t pd_base_class;
	u8_t pd_sub_class;
	u8_t pd_intf_class;
	u16_t pd_vid;
	u16_t pd_did;
	u8_t pd_in_use;
} PciDev;
static PciDev pciDev[NR_PCI_DEV];
//static int numPciDev = 0;

void disablePci() {
	int s;
	if ((s = sysOutl(PCI_CONF_ADDR, PCI_UNSEL)) != OK)
	  printf("PCI: warning, sysOutl failed: %d\n", s);
}

static u8_t pci_rd_reg8(int busInd, int devInd, int port) {
	u8_t v;

	v = PCI_RD_REG8(pciBus[busInd].pb_bus, 
				pciDev[devInd].pd_dev, 
				pciDev[devInd].pd_func, 
				port);
	disablePci();
	return v;
}

static u16_t pci_rd_reg16(int busInd, int devInd, int port) {
	u16_t v;

	v = PCI_RD_REG16(pciBus[busInd].pb_bus, 
				pciDev[devInd].pd_dev, 
				pciDev[devInd].pd_func, 
				port);
	disablePci();
	return v;
}

static u32_t pci_rd_reg32(int busInd, int devInd, int port) {
	u32_t v;

	v = PCI_RD_REG32(pciBus[busInd].pb_bus, 
				pciDev[devInd].pd_dev, 
				pciDev[devInd].pd_func, 
				port);
	disablePci();
	return v;
}

static void pci_wr_reg16(int busInd, int devInd, int port, u16_t value) {
	PCI_WR_REG16(pciBus[busInd].pb_bus, 
				pciDev[devInd].pd_dev, 
				pciDev[devInd].pd_func, 
				port, value);
	disablePci();
}

static void pci_wr_reg32(int busInd, int devInd, int port, u32_t value) {
	PCI_WR_REG32(pciBus[busInd].pb_bus, 
				pciDev[devInd].pd_dev, 
				pciDev[devInd].pd_func, 
				port, value);
	disablePci();
}

static u16_t pci_rd_status(int busInd) {
	u16_t v;

	v = PCI_RD_REG16(pciBus[busInd].pb_bus, 0, 0, PCI_STATUS);
	disablePci();
	return v;
}

static void pci_wr_status(int busInd, u16_t value) {
	PCI_WR_REG16(pciBus[busInd].pb_bus, 0, 0, PCI_STATUS, value);
	disablePci();

}

static char *pciVidName(u16_t vid) {
	int i;

	for (i = 0; pciVendorTable[i].name; ++i) {
		if (pciVendorTable[i].vid == vid)
		  return pciVendorTable[i].name;
	}
	return "unknown";
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

	bus = dev = func = 0;

	vid = PCI_RD_REG16(bus, dev, func, PCI_VID);
	did = PCI_RD_REG16(bus, dev, func, PCI_DID);
	disablePci();

	if (vid == 0xFFFF && did == 0xFFFF)
	  return;	/* Nothing here */

	for (i = 0; pciIntelCtrlTable[i].vid; ++i) {
		if (pciIntelCtrlTable[i].vid == vid &&
			pciIntelCtrlTable[i].did == did)
		  break;
	}

	if (! pciIntelCtrlTable[i].vid)
	  printf("initIntelPci (warning): unknown PCI-controller:\n"
			"\tvendor %04X (%s), device %04X\n",
			vid, pciVidName(vid), did);

	if (numPciBus >= NR_PCI_BUS)
	  panic("PCI", "too many PCI busses", numPciBus);

	busInd = numPciBus++;
	pciBus[busInd].pb_type = PBT_INTEL;
	pciBus[busInd].pb_isa_bridge_dev = -1;
	pciBus[busInd].pb_isa_bridge_type = 0;
	pciBus[busInd].pb_dev_ind = -1;
	pciBus[busInd].pb_bus = 0;
	pciBus[busInd].pb_rd_reg8 = pci_rd_reg8;
	pciBus[busInd].pb_rd_reg16 = pci_rd_reg16;
	pciBus[busInd].pb_rd_reg32 = pci_rd_reg32;
	pciBus[busInd].pb_wr_reg16 = pci_wr_reg16;
	pciBus[busInd].pb_wr_reg32 = pci_wr_reg32;
	pciBus[busInd].pb_rd_status = pci_rd_status;
	pciBus[busInd].pb_wr_status = pci_wr_status;


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
unsigned pciInb(U16_t port) {
	U8_t value;
	int s;
	if ((s = sysInb(port, &value)) != OK)
	  printf("PCI: warning, sysInb failed: %d\n", s);
	return value;
}

unsigned pciInw(U16_t port) {
	U16_t value;
	int s;
	if ((s = sysInw(port, &value)) != OK)
	  printf("PCI: warning, sysInw failed: %d\n", s);
	return value;
}

unsigned pciInl(U16_t port) {
	U32_t value;
	int s;
	if ((s = sysInl(port, &value)) != OK)
	  printf("PCI: warning, sysInl failed: %d\n", s);
	return value;
}

void pciOutb(U16_t port, U8_t value) {
	int s;
	if ((s = sysOutb(port, value)) != OK)
	  printf("PCI: warning, sysOutb failed: %d\n", s);
}

void pciOutw(U16_t port, U16_t value) {
	int s;
	if ((s = sysOutw(port, value)) != OK)
	  printf("PCI: warning, sysOutw failed: %d\n", s);
}

void pciOutl(U16_t port, U32_t value) {
	int s;
	if ((s = sysOutl(port, value)) != OK)
	  printf("PCI: warning, sysOutl failed: %d\n", s);
}




