#include "../system.h"
#include "minix/type.h"

int doCopy(register Message *msg) {
/* Handle sysVirCopy() and sysPhysCopy(). Copy data using virtual or
 * physical addressing. Although a single handler function is used, there 
 * are two different kernel calls so that permissions can be checked.
 */
	VirAddr virAddr[2];		/* Virtual source and destination address */
	phys_bytes bytes;		/* Number of bytes to copy */
	int i;

	/* Dismember the command message */
	virAddr[_SRC_].pNum = msg->CP_SRC_PROC_NR;
	virAddr[_SRC_].segment = msg->CP_SRC_SPACE;
	virAddr[_SRC_].offset = (vir_bytes) msg->CP_SRC_ADDR;
	virAddr[_DST_].pNum = msg->CP_DST_PROC_NR;
	virAddr[_DST_].segment = msg->CP_DST_SPACE;
	virAddr[_DST_].offset = (vir_bytes) msg->CP_DST_ADDR;
	bytes = (phys_bytes) msg->CP_NR_BYTES;

	/* Now do some checks for both the source and destination virtual address.
	 * This is done once for _SRC_, ehn once for _DST_.
	 */
	for (i = _SRC_; i <= _DST_; ++i) {
		/* Check if process number was given implicitly with SELF and is valid. */
		if (virAddr[i].pNum == SELF)
		  virAddr[i].pNum = msg->m_source;
		if (! isOkProcNum(virAddr[i].pNum) && virAddr[i].segment != PHYS_SEG)
		  return EINVAL;

		/* Check if physical addressing is used without SYS_PHYSCOPY. */
		if ((virAddr[i].segment & PHYS_SEG) &&
				msg->m_type != SYS_PHYSCOPY)
		  return EPERM;
	}

	/* Check for overflow. */
	if (bytes != (vir_bytes) bytes)
	  return E2BIG;
	
	/* Now try to make the actual virtual copy. */
	return virtualCopy(&virAddr[_SRC_], &virAddr[_DST_], bytes);
}

