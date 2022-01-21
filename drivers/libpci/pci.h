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

void initPci();
void disablePci();

#define PCI_VID		0x00	/* Vendor ID, 16-bit */
#define PCI_DID		0x02	/* Device ID, 16-bit */
#define PCI_CMD		0x04	/* Command register, 16-bit */
#define PCI_STATUS	0X06	/* Status register, 16-bit */

typedef struct {
	u16_t vid;
	char *name;
} PciVendor;

typedef struct {
	u16_t vid;
	u16_t did;
} PciIntelCtrl;

extern PciVendor pciVendorTable[];
extern PciIntelCtrl pciIntelCtrlTable[];

