#include "is.h"
#include "../fs/fs.h"
#include <minix/dmap.h>

FProc fprocTable[NR_PROCS];
DMap dmapTable[NR_DEVICES];


void devTabDmp() {
	int i, r;

	if ((r = getSysInfo(FS_PROC_NR, SI_DMAP_TAB, dmapTable)) != OK) {
		report("IS", "warning: couldn't get copy of fs dmap table", r);
		return;
	}

	printf("File System (FS) device <-> driver mappings\n");
	printf("Major  Proc\n");
	printf("-----  ----\n");
	for (i = 0; i < NR_DEVICES; ++i) {
		if (dmapTable[i].dmap_driver == 0)
		  continue;
		printf("%5d  ", i);
		printf("%4d\n", dmapTable[i].dmap_driver);
	}
}

void fprocDmp() {
	FProc *fp;
	int i, r, n = 0;
	static int prevIdx = 0;

	if ((r = getSysInfo(FS_PROC_NR, SI_PROC_TAB, fprocTable)) != OK) {
		report("IS", "warning: couldn't get copy of fs process table", r);
		return;
	}

	printf("File System (FS) process table dump\n");
	printf("-nr- -pid- -tty- -umask- --uid-- --gid-- -ldr- -sus-rev-proc- -cloexec-\n");

	for (i = prevIdx; i < NR_PROCS; ++i) {
		fp = &fprocTable[i];
		if (fp->fp_pid <= 0)
		  continue;
		if (++n > 22)
		  break;

		printf("%3d  %4d  %2d/%d  0x%05x %2d (%d)  %2d (%d)  %3d   %3d %3d %4d    0x%05x\n",
			i, fp->fp_pid,
			MAJOR_DEV(fp->fp_tty), MINOR_DEV(fp->fp_tty),
			fp->fp_umask,
			fp->fp_ruid, fp->fp_euid, fp->fp_rgid, fp->fp_egid,
			fp->fp_session_leader,
			fp->fp_suspended, fp->fp_revived, fp->fp_task,
			fp->fp_cloexec);
	}
	if (i >= NR_PROCS)
	  i = 0;
	else
	  printf("--more--\r");
	prevIdx = i;
}
