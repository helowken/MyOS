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

static int isHeaderInvalid(Elf32_Ehdr *ehdr) {
	return ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
			ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
			ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
			ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
			ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
			ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
			ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
			ehdr->e_type != ET_EXEC ||
			ehdr->e_phnum < 3;
}

static int readExec(int fd, Exec *prog) {
	int m, len;
	char *s;
	Elf32_Ehdr *ehdr = &prog->ehdr;

	if ((m = read(fd, ehdr, sizeof(*ehdr))) < 2)
	  return ENOEXEC;

	s = (char *) ehdr;
	if (s[0] == '#' && s[1] == '!')
	  return ESCRIPT;

	if (m != sizeof(*ehdr) || isHeaderInvalid(ehdr))
	  return ENOEXEC;

	if (lseek(fd, prog->ehdr.e_phoff, SEEK_SET) == -1)
	  return errno;

	len = prog->ehdr.e_phentsize;
	if (read(fd, &prog->codeHdr, len) != len || 
		read(fd, &prog->dataHdr, len) != len ||
		read(fd, &prog->stackHdr, len) != len ||
		! isPLoad(&prog->codeHdr) ||
		! isPLoad(&prog->dataHdr) ||
		! isStack(&prog->stackHdr))
	  return ENOEXEC;

	return OK;
}

static int readHeader(int fd, Exec *prog, vir_bytes *textBytes, 
			vir_bytes *dataBytes, vir_bytes *bssBytes, phys_bytes *totalBytes, 
			vir_clicks stackClicks, vir_clicks *offsetClicks, vir_bytes *pc) {
/* Read the header and extract the text, data, bss and total sizes from it. */
	
	vir_clicks dataClicks, dataVirAddr, stackVirAddr;
	phys_clicks totalClicks;
	int m;

	if ((m = readExec(fd, prog)) != OK)
	  return m;

	/* Get text and data sizes */
	*textBytes = prog->codeHdr.p_memsz;
	*dataBytes = prog->dataHdr.p_filesz;
	*bssBytes = prog->dataHdr.p_memsz - prog->dataHdr.p_filesz;
	*totalBytes = prog->dataHdr.p_vaddr + prog->dataHdr.p_memsz + prog->stackHdr.p_memsz;
	if (*totalBytes == 0)
	  return ENOEXEC;
	*pc = prog->ehdr.e_entry;

	dataClicks = (*dataBytes + *bssBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	totalClicks = (*totalBytes + CLICK_SIZE - 1) >> CLICK_SHIFT;
	if (dataClicks >= totalClicks)
	  return ENOEXEC;

	*offsetClicks = prog->dataHdr.p_vaddr >> CLICK_SHIFT;

	dataVirAddr = 0;
	stackVirAddr = dataVirAddr + (totalClicks - stackClicks);
	m = dataVirAddr + dataClicks > stackVirAddr ? ENOMEM : OK;
	return m;
}

static int newMem(MProc *shareMp, vir_bytes textBytes, vir_bytes dataBytes, 
		vir_bytes bssBytes, vir_bytes stackFrameBytes, phys_bytes totalBytes,
		vir_clicks offsetClicks) {
/* Allocate new memory and release the old memory. Change the map and report
 * the new map to the kernel. Zero the new core image's bss, gap and stack.
 */

	register MProc *rmp = currMp;
	vir_clicks textClicks, dataClicks, gapClicks, stackClicks, totalClicks;
	phys_clicks newBase;
	phys_bytes bytes, base;
	int s;

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
	newBase = allocMemory2(offsetClicks, textClicks + totalClicks - offsetClicks);
	if (newBase == NO_MEM)
	  return ENOMEM;

	/* We're got memory for the new core image. Release the old one. */
	if (findShare(rmp, rmp->mp_ino, rmp->mp_dev, rmp->mp_ctime) == NULL) {
		/* No other process shares the text segment, so free it. */
		freeMemory(rmp->mp_memmap[T].physAddr, rmp->mp_memmap[T].len);
	}
	/* Free the data and stack segments. */
	freeMemory(rmp->mp_memmap[D].physAddr + offsetClicks, 
				rmp->mp_memmap[D].len - offsetClicks);

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
	rmp->mp_memmap[D].physAddr = newBase + textClicks - offsetClicks;
	rmp->mp_memmap[D].virAddr = 0;
	rmp->mp_memmap[D].len = dataClicks;
	rmp->mp_memmap[D].offset = offsetClicks;
	rmp->mp_memmap[S].physAddr = rmp->mp_memmap[D].physAddr + dataClicks + gapClicks;
	rmp->mp_memmap[S].virAddr = rmp->mp_memmap[D].virAddr + dataClicks + gapClicks;
	rmp->mp_memmap[S].len = stackClicks;

	sysNewMap(who, rmp->mp_memmap);		/* Report new map to the kernel */

	/* The old memory may have been swapped out, but the new memory is real. */
	rmp->mp_flags &= ~(WAITING | ONSWAP | SWAPIN);

	/* Zero the bss, gap, and stack segment. */
	bytes = (phys_bytes) (dataClicks - offsetClicks + gapClicks + stackClicks) << CLICK_SHIFT;
	base = (phys_bytes) (rmp->mp_memmap[D].physAddr + offsetClicks) << CLICK_SHIFT;
	base += dataBytes;
	bytes -= dataBytes;

	if ((s = sysMemset(0, base, bytes)) != OK) 
	  panic(__FILE__, "newMem can't zero", s);

	return OK;
}

static void patchStack(char stack[ARG_MAX], vir_bytes base) {
/* When doing an exec(name, arg, envp) call the user builds up a stack
 * image with arg and env pointers relative to the start of the stack. Now
 * these pointers must be relocated, since the stack is not positioned at
 * address 0 in the user's address space.
 */
	char **ap, nullPtrCount;
	vir_bytes v;

	nullPtrCount = 0;	/* Counts number of 0-pointers seen */
	ap = (char **) stack;	/* Points initially to 'argc' */
	++ap;
	while (nullPtrCount < 2) {
		if (ap >= (char **) &stack[ARG_MAX])
		  return;	/* Too bad */
		if (*ap != NULL) {
			v = (vir_bytes) *ap;	/* v is relativev pointer */
			v += base;	/* Relocate it */
			*ap = (char *) v;
		} else 
		  ++nullPtrCount;
		++ap;
	}
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
	vir_bytes src, dst, stackFrameBytes, textBytes, dataBytes, bssBytes,
			  virStackBase;
	phys_bytes totalBytes;
	char *newSp, *name, *baseName;
	int m, r, fd, sn;
	struct stat stBuf[2], *stp;
	vir_clicks stackClicks, offsetClicks;
	vir_bytes pc;
	Exec prog;

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

		m = readHeader(fd, &prog, &textBytes, &dataBytes, &bssBytes, 
					&totalBytes, stackClicks, &offsetClicks, &pc);
		if (m != ESCRIPT || ++r > 1)
		  break;
	} while (false); //TODO

	if (m < 0) {
		close(fd);	/* Something wrong with header */
		return stackFrameBytes > ARG_MAX ? ENOMEM : ENOEXEC;
	}

	/* Can the process' text be shared with that of one already running? */
	shareMp = findShare(rmp, stp->st_ino, stp->st_dev, stp->st_ctime);

	/* Allocate new memory and release old memory. Fix map and tell kernel. */
	r = newMem(shareMp, textBytes, dataBytes, bssBytes, stackFrameBytes, 
				totalBytes, offsetClicks);
	if (r != OK) {
		close(fd);	/* Insufficient core or program too big */
		return r;
	}

	/* Save file identification to allow it to be shared. */
	rmp->mp_ino = stp->st_ino;
	rmp->mp_dev = stp->st_dev;
	rmp->mp_ctime = stp->st_ctime;

	/* Patch up stack and copy it from PM to new core image. */
	virStackBase = (vir_bytes) rmp->mp_memmap[S].virAddr << CLICK_SHIFT;
	virStackBase += (vir_bytes) rmp->mp_memmap[S].len << CLICK_SHIFT;
	virStackBase -= stackFrameBytes;
	patchStack(mBuf, virStackBase);
	src = (vir_bytes) mBuf;
	r = sysDataCopy(PM_PROC_NR, (vir_bytes) src,
			who, (vir_bytes) virStackBase, (phys_bytes) stackFrameBytes);
	if (r != OK)
	  panic(__FILE__, "doExec stack copy err on", who);

	/* Read in text and data segments. */
	if (shareMp == NULL) {
		lseek(fd, prog.codeHdr.p_offset, SEEK_SET);
		rwSeg(0, fd, who, T, prog.codeHdr.p_vaddr, textBytes);
	}
	if (dataBytes > 0) {
		lseek(fd, prog.dataHdr.p_offset, SEEK_SET);
		rwSeg(0, fd, who, D, prog.dataHdr.p_vaddr, dataBytes);
	}
	close(fd);		/* Don't need exec file any more */

	/* Take care of setuid/setgid bits. */
	if ((rmp->mp_flags & TRACED) == 0) {	/* Suppress if tracing */
		if (stBuf[0].st_mode & I_SET_UID_BIT) {
			rmp->mp_euid = stBuf[0].st_uid;
			tellFS(SETUID, who, (int) rmp->mp_ruid, rmp->mp_euid);
		} 
		if (stBuf[0].st_mode & I_SET_GID_BIT) {
			rmp->mp_egid = stBuf[0].st_gid;
			tellFS(SETGID, who, (int) rmp->mp_rgid, rmp->mp_egid);
		}
	}

	/* Save offset to initial argc */
	rmp->mp_proc_args = virStackBase;

	/* Fix 'MProc' fields, tell kernel that exec is done, 
	 * reset caught signals. 
	 */
	for (sn = 1; sn <= NSIG; ++sn) {
		if (sigismember(&rmp->mp_sig_catch, sn)) {
			sigdelset(&rmp->mp_sig_catch, sn);
			rmp->mp_sig_actions[sn].sa_handler = SIG_DFL;
			sigemptyset(&rmp->mp_sig_actions[sn].sa_mask);
		}
	}

	newSp = (char *) virStackBase;

	tellFS(EXEC, who, 0, 0);	/* Allow FS to handle FD_CLOEXEC files */

	/* System will save command line for debugging, ps(1) output, etc. */
	baseName = strrchr(name, '/');
	if (baseName == NULL)
	  baseName = name;
	else
	  ++baseName;
	strncpy(rmp->mp_name, baseName, PROC_NAME_LEN - 1);
	rmp->mp_name[PROC_NAME_LEN - 1] = 0;
	sysExec(who, newSp, baseName, pc);

	/* Cause a signal if this process is traced. */
	if (rmp->mp_flags & TRACED)
	  checkSig(rmp->mp_pid, SIGTRAP);

	return SUSPEND;		/* No reply, new program just runs */
}

void rwSeg(
	int rw,		/* 0 = read, 1 = write */
	int fd,		
	int pNum,	/* Process number */
	int seg,	/* T, D, or S */
	off_t dstOffset,
	phys_bytes len		/* How much is to be transferred? */
) {
/* Transfer text or data from/to a file and copy to/from a process segment.
 * This procedure is a little bit tricky. The logical way to transfer a
 * segment would be block by block and copying each block to/from the user
 * space one at a time. This is too slow, so we do something dirty here,
 * namely send the user space and virtual address to the file system in the
 * upper 10 bits of the file descriptor, and pass it the user virtual address
 * instead of a PM address. The file system extracts these parameters when
 * gets a read or write call from the process manager, which is the only
 * process that is permitted to use this trick. The file system then copies
 * the whole segment directly to/from user space, bypassing PM completely.
 *
 * The byte count on read is usually smaller than the segment count, because
 * a segment is padded out to a click multiple, and the data segment is only
 * partially initialized.
 */
	int newFd, bytes, r;
	char *buf;
	MemMap *sp = &currMp[pNum].mp_memmap[seg];
	phys_bytes segBytes = len;

	newFd = (pNum << 7) | (seg << 5) | fd;
	buf = (char *) (((vir_bytes) sp->virAddr << CLICK_SHIFT) + dstOffset);
	
	while (bytes != 0) {
#define PM_CHUNK_SIZE 8192
		bytes = MIN((INT_MAX / PM_CHUNK_SIZE) * PM_CHUNK_SIZE, segBytes);
		if (rw == 0)
		  r = read(newFd, buf, bytes);
		else 
		  r = write(newFd, buf, bytes);
		if (r != bytes)
		  break;
		buf += bytes;
		segBytes -= bytes;
	}
}




