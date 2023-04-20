#include "pm.h"
#include "sys/stat.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "signal.h"
#include "string.h"
#include "mproc.h"
#include "param.h"
#include "image.h"

#define ESCRIPT		(-2000)			/* Returned by readHeader for a '#!' script. */
#define PTR_SIZE	sizeof(char *)	/* Size of pointers in argv[[ and envp[]. */

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

static int readHeader(int fd, Exec *prog, vir_bytes *textBytes, vir_bytes *dataBytes,
			vir_bytes *bssBytes, vir_bytes *stackBytes, vir_bytes *dataVirAddr, 
			vir_bytes *pc) {
/* Read the header and extract the text, data, bss and total sizes from it. */

	phys_bytes totalBytes;
	int r;

	if ((r = readExec(fd, prog)) != OK)
	  return r;

	/* Get text and data sizes */
	*textBytes = prog->codeHdr.p_memsz;
	*dataBytes = prog->dataHdr.p_filesz;
	*bssBytes = prog->dataHdr.p_memsz - prog->dataHdr.p_filesz;
	*stackBytes = prog->stackHdr.p_memsz;
	*dataVirAddr = prog->dataHdr.p_vaddr;
	totalBytes = *dataVirAddr + *dataBytes + *bssBytes + *stackBytes;
	if (totalBytes <= 0)
	  return ENOEXEC;
	*pc = prog->ehdr.e_entry;
	return OK;
}

static int newMem(MProc *shareMp, vir_bytes textBytes, vir_bytes dataBytes, 
		vir_bytes bssBytes, vir_bytes stackBytes, vir_bytes dataVirAddr, 
		vir_bytes stackFrameBytes) {
/* Allocate new memory and release the old memory. Change the map and report
 * the new map to the kernel. Zero the new core image's bss, gap and stack.
 */

	register MProc *rmp = currMp;
	vir_bytes skipBytes;
	vir_clicks textClicks, dataClicks, gapClicks, stackFrameClicks, totalClicks;
	phys_clicks newBase, offsetClicks;
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
	textClicks = SIZE_TO_CLICKS((unsigned long) textBytes);
	dataClicks = SIZE_TO_CLICKS(dataVirAddr + dataBytes + bssBytes);
	totalClicks = SIZE_TO_CLICKS(dataVirAddr + dataBytes + bssBytes + 
								stackBytes + stackFrameBytes);
	stackFrameClicks = SIZE_TO_CLICKS(stackFrameBytes);
	offsetClicks = dataVirAddr >> CLICK_SHIFT;
	gapClicks = totalClicks - dataClicks - stackFrameClicks;
	if ((int) gapClicks < 0)	/* Check if no stack */
	  return ENOMEM;

	/* Try to allocate memory for the new process. */
	newBase = allocMemory2(offsetClicks, textClicks + totalClicks - offsetClicks);
	if (newBase == NO_MEM)
	  return ENOMEM;

	/* We're got memory for the new core image. Release the old one. */
	if (findShare(rmp, rmp->mp_ino, rmp->mp_dev, rmp->mp_ctime) == NULL) {
		/* No other process shares the text segment, so free it. */
		freeMemory(rmp->mp_seg[T].physAddr, rmp->mp_seg[T].len);
	}
	/* Free the data and stack segments. */
	freeMemory(PM_ACT_DATA_PADDR(rmp), PM_ACT_DATA_CLICKS(rmp));

	/* We have now passed the point of no return. The old core image has been
	 * forever lost, memory for a new core image has been allocated. Set up
	 * and report new map.
	 */
	if (shareMp != NULL) {
		/* Share the text segment. */
		rmp->mp_seg[T] = shareMp->mp_seg[T];
	} else {
		rmp->mp_seg[T].physAddr = newBase;
		rmp->mp_seg[T].virAddr = 0;
		rmp->mp_seg[T].len = textClicks;
	}
	rmp->mp_seg[D].physAddr = newBase + textClicks - offsetClicks;
	rmp->mp_seg[D].virAddr = 0;
	rmp->mp_seg[D].len = dataClicks;
	rmp->mp_seg[D].offset = offsetClicks;
	rmp->mp_seg[S].physAddr = rmp->mp_seg[D].physAddr + dataClicks + gapClicks;
	rmp->mp_seg[S].virAddr = rmp->mp_seg[D].virAddr + dataClicks + gapClicks;
	rmp->mp_seg[S].len = stackFrameClicks;
	//TODO printf("=== pm exec text addr: %x\n", rmp->mp_seg[T].physAddr << CLICK_SHIFT);

	sysNewMap(who, rmp->mp_seg);		/* Report new map to the kernel */

	/* The old memory may have been swapped out, but the new memory is real. */
	rmp->mp_flags &= ~(WAITING | ONSWAP | SWAPIN);

	/* Zero the bss, gap, and stack segment. */
	bytes = (phys_bytes) CLICKS_TO_SIZE(totalClicks - offsetClicks);
	base = (phys_bytes) CLICKS_TO_SIZE(rmp->mp_seg[D].physAddr + offsetClicks);
	skipBytes = dataVirAddr - CLICKS_TO_SIZE(offsetClicks) + dataBytes;
	base += skipBytes;
	bytes -= skipBytes;

	if ((s = sysMemset(0, base, bytes)) != OK) 
	  panic(__FILE__, "newMem can't zero", s);

	return OK;
}

static void patchPtr(char stack[ARG_MAX], vir_bytes base) {
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

static int insertArg(
	char stack[ARG_MAX],	/* Pointer to stack frame image within PM */
	vir_bytes *stackFrameBytes,	/* Size of initial stack frame */
	char *arg,				/* Argument to prepend/replace as new argv[0] */
	int replace				
) {
/* Patch the stack so that arg will become argv[0]. Be careful, the stack may
 * be filled with garbage, although it normally looks like this:
 *	nArgs argv[0] ... argv[nArgs - 1] NULL envp[0] ... NULL
 * followed by the strings "pointed" to by the argv[i] and the envp[i].
 * Return true iff the operation succeeded.
 */
	int offset, a0, a1, oldBytes = *stackFrameBytes;

	/* Prepending arg adds at least one string and a zero byte. */
	offset = strlen(arg) + 1;

	a0 = (int) ((char **) stack)[1];	/* argv[0] */
	/* 4 is for pointers as : argc + argv nil + envp nil + a0
	 * see execve() for more information.
	 */
	if (a0 < 4 * PTR_SIZE || a0 >= oldBytes)
	  return FALSE;

	a1 = a0;	/* a1 will point to the strings to be moved */
	if (replace) {
		/* Move a1 to the end of argv[0][] (or argv[1] if nArgs > 1).
		 * 1. argv: argvNil		(no arg, return false)
		 * 2. argv: xxx\0 argvNil	(move to the end of argv[0])
		 * 3. argv: xxx\0 yyy\0 argvNil	(mvoe to argv[1])
		 */
		do {
			if (a1 == oldBytes)
			  return FALSE;
			--offset;
		} while (stack[a1++] != 0);
	} else {
		offset += PTR_SIZE;	/* New argv[0] needs new pointer in argv[] */
		a0 += PTR_SIZE;		/* Location of new argv[0][]. */
	}

	/* Stack will grow by offset bytes (or shrink by -offset bytes) */
	if ((*stackFrameBytes += offset) > ARG_MAX)
	  return FALSE;

	/* Reposition the strings by offset bytes. */
	memmove(stack + a1 + offset, stack + a1, oldBytes - a1);

	strcpy(stack + a0, arg);	/* Put arg in the new space. */

	if (! replace) {
		/* Make space for new argv[0].
		 * No need to move the location of argc.
		 */
		memmove(stack + 2 * PTR_SIZE, stack + 1 * PTR_SIZE, a0 - 2 * PTR_SIZE);

		++((char **) stack)[0];	/* ++nArgs; */
	}

	/* Now patch up argv[] and envp[] by offset. */
	patchPtr(stack, (vir_bytes) offset);
	((char **) stack)[1] = (char *) a0;		/* Set argv[0] correctly */
	return TRUE;
}

static char *patchStack(
	int fd,					/* File descriptor to open script file */
	char stack[ARG_MAX],	/* Pointer to stack frame image within PM */
	vir_bytes *stackFrameBytes, /* Size of initial stack frame */
	char *script			/* Name of script to interpret */
) {
/* Patch the argument vector to include the path name of the script to be
 * interpreted, and all strings on the #! line. Returns the patch name of
 * the interpreter.
 */
	char *sp, *interp = NULL;
	int n;
	enum { INSERT = FALSE, REPLACE = TRUE };

	/* Make script[] the new argv[0]. */
	if (! insertArg(stack, stackFrameBytes, script, REPLACE))
	  return NULL;

	if (lseek(fd, 2L, 0) == -1 ||	/* Just behind the #! */
			(n = read(fd, script, PATH_MAX)) < 0 ||	/* Read line one */
			(sp = memchr(script, '\n', n)) == NULL)	/* Must be proper line */
	  return NULL;

	/* Move sp backwards through script[], prepending each string to stack. */
	while (TRUE) {
		/* Skip spaces behind argument. */
		while (sp > script && (*--sp == ' ' || *sp == '\t')) {
		}
		if (sp == script)	/* No shell arguments */
		  break;

		sp[1] = 0;	/* Replace '\n' or ' ' or '\t' with 0 */

		/* Move to the start of the argument */
		while (sp > script && sp[-1] != ' ' && sp[-1] != '\t') {
			--sp;
		}
		
		interp = sp;
		if (! insertArg(stack, stackFrameBytes, sp, INSERT))
		  return NULL;
	}

	/* Round *stackFrameBytes up to the size of a pointer for alignment contraints. */
	*stackFrameBytes = ((*stackFrameBytes + PTR_SIZE - 1) / PTR_SIZE) * PTR_SIZE;

	close(fd);
	return interp;
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
			  stackBytes, dataVirAddr, virStackBase;
	char *newSp, *name, *baseName;
	int m, r, fd, sn;
	struct stat stBuf[2], *stp;
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
		m = readHeader(fd, &prog, &textBytes, &dataBytes, &bssBytes, &stackBytes, 
					&dataVirAddr, &pc);
		if (m != ESCRIPT || ++r > 1)
		  break;
	} while ((name = patchStack(fd, mBuf, &stackFrameBytes, nameBuf)) != NULL);

	if (m < 0) {
		close(fd);	/* Something wrong with header */
		return stackFrameBytes > ARG_MAX ? ENOMEM : ENOEXEC;
	}

	/* Can the process' text be shared with that of one already running? */
	shareMp = findShare(rmp, stp->st_ino, stp->st_dev, stp->st_ctime);

	/* Allocate new memory and release old memory. Fix map and tell kernel. */
	r = newMem(shareMp, textBytes, dataBytes, bssBytes, stackBytes, 
				dataVirAddr, stackFrameBytes);
	if (r != OK) {
		close(fd);	/* Insufficient core or program too big */
		return r;
	}

	/* Save file identification to allow it to be shared. */
	rmp->mp_ino = stp->st_ino;
	rmp->mp_dev = stp->st_dev;
	rmp->mp_ctime = stp->st_ctime;

	/* Patch up stack and copy it from PM to new core image. */
	virStackBase = (vir_bytes) rmp->mp_seg[S].virAddr << CLICK_SHIFT;
	virStackBase += (vir_bytes) rmp->mp_seg[S].len << CLICK_SHIFT;
	virStackBase -= stackFrameBytes;
	patchPtr(mBuf, virStackBase);
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
		rwSeg(0, fd, who, D, dataVirAddr, dataBytes);
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
	MemMap *sp = &currMp[pNum].mp_seg[seg];
	phys_bytes segBytes = len;

	newFd = (pNum << 7) | (seg << 5) | fd;
	buf = (char *) (((vir_bytes) sp->virAddr << CLICK_SHIFT) + dstOffset);
	
	while (segBytes != 0) {
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




