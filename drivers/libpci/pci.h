u8_t PCI_RD_REG8(int bus, u8_t dev, u8_t func, int reg);
u16_t PCI_RD_REG16(int bus, u8_t dev, u8_t func, int reg);
u32_t PCI_RD_REG32(int bus, u8_t dev, u8_t func, int reg);

void PCI_WR_REG8(int bus, u8_t dev, u8_t func, int reg, u8_t value);
void PCI_WR_REG16(int bus, u8_t dev, u8_t func, int reg, u16_t value);
void PCI_WR_REG32(int bus, u8_t dev, u8_t func, int reg, u32_t value);

unsigned pciInb(U16_t port);
unsigned pciInw(U16_t port);
unsigned pciInl(U16_t port);

void pciOutb(U16_t port, U8_t value);
void pciOutw(U16_t port, U16_t value);
void pciOutl(U16_t port, U32_t value);

u8_t pciAttrR8(int devInd, int port);
u16_t pciAttrR16(int devInd, int port);
u32_t pciAttrR32(int devInd, int port);
void pciAttrW16(int devInd, int port, U16_t value);
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

