#include "pm.h"
#include "signal.h"
#include "mproc.h"
#include "param.h"

#define DATA_CHANGED	1	/* Flag value when data segment size changed */
#define STACK_CHANGED	2	/* Flag value when stack size changed */

int doBrk() {
/* Perform the brk(addr) system call.
 *
 * The parameter, 'addr' is the new virtual address in D space.
 */
	register MProc *rmp;
	int r;
	vir_bytes v, newSp;
	vir_clicks newClicks;

	rmp = currMp;
	v = (vir_bytes) inMsg.m_addr;
	newClicks = (vir_clicks) (((long) v + CLICK_SIZE - 1) >> CLICK_SHIFT);
	if (newClicks < rmp->mp_memmap[D].virAddr) {
		rmp->mp_reply.m_reply_ptr = (char *) -1;
		return ENOMEM;
	}
	newClicks -= rmp->mp_memmap[D].virAddr;
	if ((r = getStackPtr(who, &newSp)) != OK)	/* Ask kernel for sp value */
	  panic(__FILE__, "couldn't get stack pointer", r);
	r = adjust(rmp, newClicks, newSp);
	rmp->mp_reply.m_reply_ptr = (r == OK ? inMsg.m_addr : (char *) - 1);
	return r;
}

int adjust(
	register MProc *rmp,	/* Whose memory is being adjusted? */
	vir_clicks dataClicks,	/* How big is data segment to become? */
	vir_bytes sp		/* New value of esp */
) {
/* See if data and stack segments can coexist, adjusting them if need be.
 * Memory is never allocated or freed. Instead it is added or removed from the
 * gap between data segment and stack segment. If the gap size becomes
 * negative, the adjustment of data or stack fails and ENOMEM is returned.
 */
	register MemMap *memSp, *memDp;
	vir_clicks spClick, gapBase, lower, oldClicks;
	int changed;
	long baseOfStack, delta;
	int r;

	memDp = &rmp->mp_memmap[D];	/* Pointer to data segment map */
	memSp = &rmp->mp_memmap[S];	/* Pointer to stack segment map */
	changed = 0;		/* Set when either segment changed */

	if (memSp->len == 0)
	  return OK;	/* Don't bother init */

	/* See if stack size has gone negative (i.e., sp too close to 0xFFFF...) */
	baseOfStack = (long) memSp->virAddr + (long) memSp->len;	/* len = stack frame size */
	spClick = sp >> CLICK_SHIFT;	/* Click containing sp */
	if (spClick >= baseOfStack)
	  return ENOMEM;	/* Sp too high */

	/* Compute size of gap between stack and data segments. */
	delta = (long) memSp->virAddr - (long) spClick;
	lower = (delta > 0 ? spClick : memSp->virAddr);

	/* Add a safety margin for future stack growth. Impossible to do right. */
#define SAFETY_BYTES	(384 * sizeof(char *))
#define SAFETY_CLICKS	((SAFETY_BYTES + CLICK_SIZE - 1) / CLICK_SIZE)
	gapBase = memDp->virAddr + dataClicks + SAFETY_CLICKS;
	if (lower < gapBase)
	  return ENOMEM;	/* Data and stack collided */

	/* Update data length (but not data origin) on behalf of brk() system call. */
	oldClicks = memDp->len;
	if (dataClicks != oldClicks) {
		memDp->len = dataClicks;
		changed |= DATA_CHANGED;
	}

	/* Update stack length and origin due to change in stack pointer. */
	if (delta > 0) {
		memSp->virAddr -= delta;
		memSp->physAddr -= delta;
		memSp->len += delta;
		changed |= STACK_CHANGED;
	}

	/* Do the new data and stack segment sizes fit in the address space? */
	r = (memDp->virAddr + memDp->len > memSp->virAddr) ? ENOMEM : OK;
	if (r == OK) {
		if (changed)
		  sysNewMap((int) (rmp - mprocTable), rmp->mp_memmap);
		return OK;
	}

	/* New sizes don't fit or require too many page/segment registers. Restore. */
	if (changed & DATA_CHANGED)
	  memDp->len = oldClicks;
	if (changed & STACK_CHANGED) {
		memSp->virAddr += delta;
		memSp->physAddr += delta;
		memSp->len -= delta;
	}
	return ENOMEM;	
}
