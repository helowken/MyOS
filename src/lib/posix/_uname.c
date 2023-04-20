/* Return information about the Minix system. Alas most
 * of it is gathered at compile time, so machine is wrong, and
 * release and version become wrong if not recompiled.
 * More chip types and Minix versions need to be added.
 */
#include "sys/types.h"
#include "sys/utsname.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "errno.h"
#include "minix/config.h"
#include "minix/com.h"
#include "minix/minlib.h"

int uname(struct utsname *name) {
	int fd, n, err;
	KernelInfo kernelInfo;
	char *nl;

	/* Read the node name from /etc/hostname.file. */
	if ((fd = open("/etc/hostname.file", O_RDONLY)) < 0) {
		if (errno != ENOENT)
		  return -1;
		strcpy(name->nodename, "noname");
	} else {
		n = read(fd, name->nodename, sizeof(name->nodename) - 1);
		err = errno;
		close(fd);
		errno = err;
		if (n < 0)
		  return -1;
		name->nodename[n] = '\0';
		if ((nl = strchr(name->nodename, '\n')) != NULL) 
		  memset(nl, 0, (name->nodename + sizeof(name->nodename)) - nl);
	}

	getSysInfo(PM_PROC_NR, SI_KINFO, &kernelInfo);

	strcpy(name->sysname, "Minix");
	strcpy(name->release, kernelInfo.release);
	strcpy(name->version, kernelInfo.version);
	name->machine[0] = 'i';
	strcpy(name->machine + 1, itoa(getProcessor()));
	strcpy(name->arch, "i386");
	return 0;
}

