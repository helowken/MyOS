#ifndef _BIOS_H
#define _BIOS_H

/* PART I --
 * The BIOS interrupt vector table (IVT) area (1024 B as of address 0x0000).
 * Although this area holds 256 interrupt vectors (with jump addresses), some
 * vectors actually contain important BIOS data. Some addresses are below.
 */
#define BIOS_HD0_PARAMS_ADDR	0x0104	/* Disk 0 parameters */
#define BIOS_HD0_PARAMS_SIZE	4L

#define BIOS_HD1_PARAMS_ADDR	0x0118	/* Disk 1 parameters */
#define BIOS_HD1_PARAMS_SIZE	4L

/* Hard disk parameters. */
#define NR_HD_DRIVES_ADDR		0x475	/* Number of hard disk drives */
#define NR_HD_DRIVES_SIZE		1L

#endif

