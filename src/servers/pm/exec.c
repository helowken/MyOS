#include "pm.h"
#include "sys/stat.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "signal.h"
#include "string.h"
#include "mproc.h"
#include "param.h"
#include "image.h"

#define ESCRIPT	(-2000)		/* Returned by readHeader for a '#!' script. */

MProc *findShare(MProc *rmp, ino_t ino, dev_t dev, time_t ctime) {
/* Look for a process that is the file <ino, dev, ctime> in execute. Don't
 * accidentally "find" rmp, because it is the process on whose behalf this
 * call is made.
 */
	MProc *shareMp;
	for (shareMp = &mprocTable[0]; shareMp < &mprocTable[NR_PROCS]; ++ shareMp) {
		if (shareMp == rmp ||
			shareMp->mp_ino != ino ||
			shareMp->mp_dev != dev ||
			shareMp->mp_ctime != ctime)
		  continue;

		return shareMp;
	}
	return NULL;
}

static int isHeaderInvalid(Exec *prog) {
	Elf32_Ehdr *ehdrPtr = &prog->ehdr;

	return ehdrPtr->e_ident[EI_MAG0] != ELFMAG0 ||
			ehdrPtr->e_ident[EI_MAG1] != ELFMAG1 ||
			ehdrPtr->e_ident[EI_MAG2] != ELFMAG2 ||
			ehdrPtr->e_ident[EI_MAG3] != ELFMAG3 ||
			ehdrPtr->e_ident[EI_CLASS] != ELFCLASS32 ||
			ehdrPtr->e_ident[EI_DATA] != ELFDATA2LSB ||
			ehdrPtr->e_ident[EI_VERSION] != EV_CURRENT ||
			ehdrPtr->e_type != ET_EXEC ||
			! isPLoad(&prog->codeHdr) ||
			! isPLoad(&prog->dataHdr) ||
			! isStack(&prog->stackHdr);
}

static int readHeader(int fd, vir_bytes *textBytes, vir_bytes *dataBytes,
			vir_bytes *bssBytes, phys_bytes *totalBytes, 
			vir_clicks stackClicks, vir_bytes *pc) {
/* Read the header and extract the text, data, bss and total sizes from it. */

	vir_clicks dataClicks, dataVirAddr, stackVirAddr;
	phys_clicks totalClicks;
	Exec prog;
	int m;
	char *s;

	if ((m = read(fd, &prog, sizeof(prog))) < 2)
	  return ENOEXEC;

	s = (char *) &prog;
	if (s[0] == '#' && s[1] == '!')
	  return ESCRIPT;

	if (m != sizeof(prog) || isHeaderInvalid(&prog))
	  return ENOEXEC;

	/* Get text and data sizes */
	*textBytes = prog.codeHdr.p_memsz;
	*dataBytes = prog.dataHdr.p_filesz;
	*bssBytes = prog.dataHdr.p_memsz - prog.dataHdr.p_filesz;
	*totalBytes = prog.dataHdr.p_vaddr + prog.dataHdr.p_memsz + prog.stackHdr.p_memsz;
	if (*totalBytes == 0)
	  return ENOEXEC;
	*pc = prog.ehdr.e_entry;

	dataClicks = (*dataBytes + *bssBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	totalClicks = (*totalBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	if (dataClicks >= totalClicks)
	  return ENOEXEC;

	dataVirAddr = 0;
	stackVirAddr = dataVirAddr + (totalClicks - stackClicks);
	m = dataVirAddr + dataClicks > stackVirAddr ? ENOMEM : OK;
	return m;
}

 char *patchStack(//TODO static
	int fd,					/* File descriptor to open script file */ 
	char stack[ARG_MAX],	/* Pointer to stack image within PM */
	vir_bytes *stackFrameBytes,	/* Size of initial stack */
	char *script			/* Name of script to interpret */
) {
/* Patch the argument vector to include the path name of the script to be
 * interpreted, and all strings on the #! line. Returns the path name of
 * the interpreter.
 */
	 return NULL;
}

static int newMem(MProc *shareMp, vir_bytes textBytes, vir_bytes dataBytes, 
		vir_bytes bssBytes, vir_bytes stackFrameBytes, phys_bytes totalBytes) {
/* Allocate new memory and release the old memory. Change the map and report
 * the new map to the kernel. Zero the new core image's bss, gap and stack.
 */

	register MProc *rmp = currMp;
	vir_clicks textClicks, dataClicks, gapClicks, stackClicks, totalClicks;
	phys_clicks newBase;

	/* No need to allocate text if it can be shared. */
	if (shareMp != NULL)
	  textBytes = 0;
	
	/* Allow the old data to be swapped out to make room. (Which is realy a
	 * waste of time, because we are going to throw it away anyway.)
	 */
	rmp->mp_flags |= WAITING;

	/* Acquire the new memory. Each of the 4 parts: text, (data+bss), gap,
	 * and stack occupies an integral number of clicks, starting at click
	 * boundary. The data and bass parts are run together with no space.
	 */
	textClicks = ((unsigned long) textBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	dataClicks = (dataBytes + bssBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	stackClicks = (stackFrameBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	totalClicks = (totalBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	gapClicks = totalClicks - dataClicks - stackClicks;
	if ((int) gapClicks < 0)
	  return ENOMEM;

	/* Try to allocate memory for the new process. */
	newBase = allocMemory(textClicks + totalClicks);
	if (newBase == NO_MEM)
	  return ENOMEM;

	/* We're got memory for the new core image. Release the old one. */
	if (findShare(rmp, rmp->mp_ino, rmp->mp_dev, rmp->mp_ctime) == NULL) {
		/* No other process shares the text segment, so free it. */
		freeMemory(rmp->mp_memmap[T].physAddr, rmp->mp_memmap[T].len);
	}
	/* Free the data and stack segments. */
	freeMemory(rmp->mp_memmap[D].physAddr, rmp->mp_memmap[D].len);

	/* We have now passed the point of no return. The old core image has been
	 * forever lost, memory for a new core image has been allocated. Set up
	 * and report new map.
	 */
	if (shareMp != NULL) {
		/* Share the text segment. */
		rmp->mp_memmap[T] = shareMp->mp_memmap[T];
	} else {
		rmp->mp_memmap[T].physAddr = newBase;
		rmp->mp_memmap[T].virAddr = 0;
		rmp->mp_memmap[T].len = textClicks;
	}
	//rmp->mp_memmap[D].physAddr = newBase//TODO
	return 0;
}

int doExec() {
/* Perform the execve(name, argv, envp) call. The user library builds a
 * complete stack image, including pointers, args, environ, etc. The stack
 * is copied to a buffer inside PM, and then to the new core image.
 */
	register MProc *rmp;
	MProc *shareMp;
	static char mBuf[ARG_MAX];		/* Buffer for stack and zeroes */
	static char nameBuf[PATH_MAX];	/* The name of the file to exec */
	vir_bytes src, dst, stackFrameBytes, textBytes, dataBytes, bssBytes;
	phys_bytes totalBytes;
	char *name;
	int m, r, fd;
	struct stat stBuf[2], *stp;
	vir_clicks stackClicks;
	vir_bytes pc;

	/* Do some validity checks. */
	rmp = currMp;
	stackFrameBytes = (vir_bytes) inMsg.m_stack_bytes;
	if (stackFrameBytes > ARG_MAX)
	  return ENOMEM;	/* Stack too big */
	if (inMsg.m_exec_len <= 0 || inMsg.m_exec_len > PATH_MAX)
	  return EINVAL;

	/* Get the exec file name and see if the file is executable. */
	src = (vir_bytes) inMsg.m_exec_name;
	dst = (vir_bytes) nameBuf;
	if ((r = sysDataCopy(who, src, PM_PROC_NR, dst,
				(phys_bytes) inMsg.m_exec_len)) != OK)
	  return r;		/* File name not in user data segment */
	
	/* Fetch the stack from the user before destroying the old core image. */
	src = (vir_bytes) inMsg.m_stack_ptr;
	dst = (vir_bytes) mBuf;
	if ((r = sysDataCopy(who, src, 
				PM_PROC_NR, dst, (phys_bytes) stackFrameBytes)) != OK)
	  return EACCES;		/* Can't fetch stack (e.g. bad virtual addr) */

	r = 0;	/* r = 0 (frist attemp), or 1 (interpreted script) */
	name = nameBuf;
	do {
		stp = &stBuf[r];
		tellFS(CHDIR, who, false, 0);	/* Switch to the user's FS environ */
		fd = checkAllowed(name, stp, X_BIT);	/* Is file executable? */
		if (fd < 0)
		  return fd;	/* File was not executable */
		
		/* Read the file header and extract the segment sizes. */
		stackClicks = (stackFrameBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;

		m = readHeader(fd, &textBytes, &dataBytes, &bssBytes, &totalBytes, 
					stackClicks, &pc);
		if (m != ESCRIPT || ++r > 1)
		  break;
	} while (false); //TODO

	printf("==== text: %x, data: %x, bss: %x, total: %x, sc: %x, pc: %x\n",
				textBytes, dataBytes, bssBytes, totalBytes, stackClicks, pc);//TODO

	if (m < 0) {
		close(fd);	/* Something wrong with header */
		return stackFrameBytes > ARG_MAX ? ENOMEM : ENOEXEC;
	}

	/* Can the process' text be shared with that of one already running? */
	shareMp = findShare(rmp, stp->st_ino, stp->st_dev, stp->st_ctime);

	/* Allocate new memory and release old memory. Fix map and tell kernel. */
	r = newMem(shareMp, textBytes, dataBytes, bssBytes, stackFrameBytes, totalBytes);

	return EACCES; //TODO
}
