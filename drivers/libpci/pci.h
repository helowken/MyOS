unsigned pciInb(u16_t port);
unsigned pciInw(u16_t port);
unsigned pciInl(u16_t port);

void pciOutb(u16_t port, u8_t value);
void pciOutw(u16_t port, u16_t value);
void pciOutl(u16_t port, u32_t value);

u8_t pciAttrDevR8(int devInd, int port);
u16_t pciAttrDevR16(int devInd, int port);
u32_t pciAttrDevR32(int devInd, int port);
void pciAttrDevW16(int devInd, int port, u16_t value);
void pciAttrDevW32(int devInd, int port, u32_t value);

/* pci.c */
void initPci();
void disablePci();
int pciFirstAvailDev(int *devIndPtr, u16_t *vidPtr, u16_t *didPtr);
int pciNextAvailDev(int *devIndPtr, u16_t *vidPtr, u16_t *didPtr);

#define PCI_VID		0x00	/* Vendor ID, 16-bit */
#define PCI_DID		0x02	/* Device ID, 16-bit */
#define PCI_CMD		0x04	/* Command register, 16-bit */
#define PCI_STATUS	0X06	/* Status register, 16-bit */
#define		PSR_SSE		0x4000	/* Signaled System Error */
#define		PSR_RMAS	0x2000	/* Received Master Abort Status */
#define		PSR_RTAS	0x1000	/* Received Target Abort Status */
#define PCI_HEADER_TYPE	0x0E	/* Header type, 8-bit */
#define		PHT_MULTI_FUNC	0x80	/* Multiple functions */
#define	PCI_PROG_IF		0x09	/* Prog. Interface Register */
#define PCI_SUB_CLASS	0x0A	/* Sub-Class Register */
#define PCI_BASE_CLASS	0x0B	/* Base-Class Register */
#define PCI_INTR_LINE	0x3C	/* Interrupt Line Register */

#define NO_VID		0xffff	/* No PCI card present */
#define NO_DID		0xffff	/* No PCI card present */

/* PCI bridge devices (AGP) */
#define PPB_SUB_BUS_NR	0x19	/* Secondary Bus Number */

/* Intel compatible PCI bridge devices (AGP) */
#define PPB_SEC_STATUS	0x1E	/* Secondary PCI-to-PCI Status Register */

typedef struct {
	u16_t vid;
	char *name;
} PciVendor;

typedef struct {
	u16_t vid;
	u16_t did;
	char *name;
} PciDevItem;

typedef struct {
	u16_t vid;
	u16_t did;
} PciIntelCtrl;

typedef struct {
	u8_t baseClass;
	char *name;
} PciBaseClass;

typedef struct {
	u8_t baseClass;
	u8_t subClass;
	u8_t intfClass;
	char *name;
} PciSubClass;

typedef struct {
	u16_t vid;
	u16_t did;
	int checkClass;
	int type;
} PciIsaBridge;

typedef struct {
	u16_t vid;
	u16_t did;
	int type;
} PciPciBridge;

#define PCI_IB_PIIX	1	/* Intel PIIX compatible ISA bridge */
#define PCI_IB_VIA	2	/* VIA compatible ISA bridge */
#define PCI_IB_AMD	3	/* AMD compatible ISA bridge */
#define PCI_IB_SIS	4	/* SIS compatible ISA bridge */

#define PCI_PCIB_INTEL 1	/* Intel compatible PCI bridge */
#define PCI_AGPB_INTEL 2	/* Intel compatible AGP bridge */

extern PciVendor pciVendorTable[];
extern PciDevItem pciDeviceTable[];
extern PciIntelCtrl pciIntelCtrlTable[];
extern PciBaseClass pciBaseClassTable[];
extern PciSubClass pciSubClassTable[];
extern PciIsaBridge pciIsaBridgeTable[];
extern PciPciBridge pciPciBridgeTable[];

