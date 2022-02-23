#include "fs.h"
#include "fproc.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"
#include "unistd.h"
#include "minix/com.h"
#include "param.h"

/* Some devices may or may not be there in the next table. */
#define DT(enable, openClose, io, driver, flags) \
	{ (enable ? (openClose) : noDev), \
	  (enable ? (io) : 0), \
	  (enable ? (driver) : 0), \
	  (flags) \
	},

/* The order of the entries here determines the mapping between major device
 * numbers and tasks. The first entry (major device 0) is not used. The next
 * entry is major devie 1, etc. Character and block devices can be intermixed
 * at random. The ordering determines the device numbers in /dev/.
 * Note that FS knows the device number of /dev/ram/ to load the RAM disk.
 * Also note that the major device numbers used in /dev/ are NOT the same as
 * the process numbers of the device drivers.
 */
/*
 * Driver enabled	Open/Close	I/O		Driver #	Flags		   Device   File
 * --------------	----------	---		--------	-----		   ------   ----
 */
DMap initDMap[] = {
	DT(1,			noDev,		0,		0,			 0)			   /* 0  =  not used    */
	DT(1,			genOpCl,	genIO,	MEM_PROC_NR, 0)			   /* 1  =  /dev/mem    */
	DT(0,			noDev,		0,		0,			 DMAP_MUTABLE) /* 2  =  /dev/fd0    */
	DT(0,			noDev,		0,		0,			 DMAP_MUTABLE) /* 3  =  /dev/c0     */
	DT(1,			ttyOpCl,	genIO,	TTY_PROC_NR, 0)			   /* 4  =  /dev/tty00  */
	DT(1,			cttyOpCl,	cttyIO,	TTY_PROC_NR, 0)			   /* 5  =  /dev/tty    */
	DT(0,			noDev,		0,		NONE,		 DMAP_MUTABLE) /* 6  =  /dev/lp     */

	DT(1,			noDev,		0,		0,			 DMAP_MUTABLE) /* 7  =  /dev/ip     */
	DT(0,			noDev,		0,		NONE,		 DMAP_MUTABLE) /* 8  =  /dev/c1     */
	DT(0,			0,			0,		0,			 DMAP_MUTABLE) /* 9  =  not used    */
	DT(0,			noDev,		0,		0,			 DMAP_MUTABLE) /* 10 =  /dev/c2     */
	DT(0,			0,			0,		0,			 DMAP_MUTABLE) /* 11 =  not used    */
	DT(0,			noDev,		0,		NONE,		 DMAP_MUTABLE) /* 12 =  /dev/c3     */
	DT(0,			noDev,		0,		NONE,		 DMAP_MUTABLE) /* 13 =  /dev/audio  */
	DT(0,			noDev,		0,		NONE,		 DMAP_MUTABLE) /* 14 =  /dev/mixer  */
	DT(1,			genOpCl,	genIO,	LOG_PROC_NR, 0)			   /* 15 =  /dev/klog   */
	DT(0,			noDev,		0,		NONE,		 DMAP_MUTABLE) /* 16 =  /dev/random */
	DT(0,			noDev,		0,		NONE,		 DMAP_MUTABLE) /* 17 =  /dev/cmos   */
};

DMap dmapTable[NR_DEVICES];

static int mapDriver(int major, int pNum, int style) {
/* Set a new device driver mapping in the dmap table. Given that correct
 * arguments are given, this only works if the entry is mutable and the
 * current driver is not busy.
 * Normal error codes are returned so that this function can be used from
 * a system call that tries to dynamically install a new driver.
 */
	DMap *dp;
	
	if (major >= NR_DEVICES)
	  return ENODEV;
	dp = &dmapTable[major];

	/* See if updating the entry is allowed. */
	if (! (dp->dmap_flags & DMAP_MUTABLE))
	  return EPERM;
	if (dp->dmap_flags & DMAP_BUSY)
	  return EBUSY;

	/* Check process number of new driver. */
	if (! isOkProcNum(pNum))
	  return EINVAL;

	/* Try to update the entry. */
	switch (style) {
		case STYLE_DEV:
			dp->dmap_opcl = genOpCl;
			break;
		case STYLE_TTY:
			dp->dmap_opcl = ttyOpCl;
			break;
		case STYLE_CLONE:
			dp->dmap_opcl = cloneOpCl;
			break;
		default:
			return EINVAL;
	}
	dp->dmap_io = genIO;
	dp->dmap_driver = pNum;
	return OK;
}

void buildDMap() {
/* Initialize the table with all device <-> driver mapping. Then, map
 * the boot driver to a controller and update the devMap table to that
 * selection. The boot driver and the controller it handles are set at
 * the boot monitor.
 */
	char driver[16];
	char *controller = "c##";
	int num, major = -1;
	DMap *dp;
	int i, s;

	/* Build table with device <-> driver mappings. */
	for (i = 0; i < NR_DEVICES; ++i) {
		dp = &dmapTable[i];
		if (i < sizeof(initDMap)/sizeof(DMap) &&
				initDMap[i].dmap_opcl != noDev) {	/* A present driver */
			dp->dmap_opcl = initDMap[i].dmap_opcl;
			dp->dmap_io = initDMap[i].dmap_io;
			dp->dmap_driver = initDMap[i].dmap_driver;
			dp->dmap_flags = initDMap[i].dmap_flags;
		} else {	/* No default */
			dp->dmap_opcl = noDev;
			dp->dmap_io = 0;
			dp->dmap_driver = 0;
			dp->dmap_flags = DMAP_MUTABLE;
		}
	}

	/* Get settings of 'controller' and 'driver' at the boot monitor. */
	if ((s = envGetParam("label", driver, sizeof(driver))) != OK)
	  panic(__FILE__, "couldn't get boot monitor parameter 'driver'", s);
	if ((s = envGetParam("controller", controller, sizeof(controller))) != OK)
	  panic(__FILE__, "couldn't get boot monitor paramter 'controller'", s);

	/* Determine major number to map driver onto. */
	if (controller[0] == 'f' && controller[1] == 'd') {
	    major = FLOPPY_MAJOR;
	} else if (controller[0] == 'c' && isDigit(controller[1])) {
		if ((num = (unsigned) atoi(&controller[1])) > NR_CTRLRS) 
		  panic(__FILE__, "monitor 'controller' maximum 'c#' is", NR_CTRLRS);
		major = CTRLR(num);
	} else {
		panic(__FILE__, "monitor 'controller' syntax is 'c#' or 'fd'", NO_NUM);
	}

	/* Now try to set the actual mapping and report to the user. */
	if ((s = mapDriver(major, DRVR_PROC_NR, STYLE_DEV)) != OK)
	  panic(__FILE__, "mapDriver failed", s);

	printf("Boot medium driver: %s driver mapped onto controller %s.\n", 
				driver, controller);
}
