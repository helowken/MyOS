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

/* Video controller (VDU). */
#define VDU_SCREEN_COLS_ADDR	0x44A	/* VDU num of screen columns */
#define VDU_SCREEN_COLS_SIZE	2L
#define VDU_SCREEN_ROWS_ADDR	0x484	/* Screen rows (less 1, EGA+) */
#define VDU_SCREEN_ROWS_SIZE	1L
#define VDU_FONTLINES_ADDR		0x485	/* Point height of char matrix */
#define VDU_FONTLINES_SIZE		2L
#define VDU_VIDEO_MODE_ADDR		0x49A	/* Current video mode */
#define VDU_VIDEO_MODE_SIZE		1L

/* Base I/O port address for active 6845 CRT controller. */
#define VDU_CRT_BASE_ADDR		0x463	/* 3B4h = mono, 3D4h = color */
#define VDU_CRT_BASE_SIZE		2L

#endif

