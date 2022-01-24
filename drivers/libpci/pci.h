unsigned pciInb(u16_t port);
unsigned pciInw(u16_t port);
unsigned pciInl(u16_t port);

void pciOutb(u16_t port, u8_t value);
void pciOutw(u16_t port, u16_t value);
void pciOutl(u16_t port, u32_t value);

u8_t pciAttrR8(int devInd, int port);
u16_t pciAttrR16(int devInd, int port);
u32_t pciAttrR32(int devInd, int port);
void pciAttrW16(int devInd, int port, u16_t value);
void pciAttrW32(int devInd, int port, u32_t value);

void initPci();
void disablePci();

#define PCI_VID		0x00	/* Vendor ID, 16-bit */
#define PCI_DID		0x02	/* Device ID, 16-bit */
#define PCI_CMD		0x04	/* Command register, 16-bit */
#define PCI_STATUS	0X06	/* Status register, 16-bit */
#define		PSR_SSE		0x4000	/* Signaled System Error */
#define		PSR_RMAS	0x2000	/* Received Master Abort Status */
#define		PSR_RTAS	0x1000	/* Received Target Abort Status */
#define PCI_HEADER_TYPE	0x0E	/* Header type, 8-bit */
#define	PCI_PROG_IF		0x09	/* Prog. Interface Register */
#define PCI_SUB_CLASS	0x0A	/* Sub-Class Register */
#define PCI_BASE_CLASS	0x0B	/* Base-Class Register */

#define NO_VID		0xffff	/* No PCI card present */
#define NO_DID		0xffff	/* No PCI card present */

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

extern PciVendor pciVendorTable[];
extern PciDevItem pciDeviceTable[];
extern PciIntelCtrl pciIntelCtrlTable[];
extern PciBaseClass pciBaseClassTable[];
extern PciSubClass pciSubClassTable[];

