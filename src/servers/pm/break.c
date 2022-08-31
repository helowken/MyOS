#include "pm.h"
#include "signal.h"
#include "mproc.h"
#include "param.h"


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
	v = (vir_bytes) inMsg.addr;
	newClicks = (vir_clicks) (((long) v + CLICK_SIZE - 1) >> CLICK_SHIFT);
	if (newClicks < rmp->mp_memmap[D].virAddr) {
		rmp->mp_reply.reply_ptr = (char *) -1;
		return ENOMEM;
	}
	newClicks -= rmp->mp_memmap[D].virAddr;
	if ((r = getStackPtr(who, &newSp)) != OK)	/* Ask kernel for sp value */
	  panic(__FILE__, "couldn't get stack pointer", r);
	r = adjust(rmp, newClicks, newSp);
	rmp->mp_reply.reply_ptr = (r == OK ? inMsg.addr : (char *) - 1);
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
	register MemMap *memSp;
	//int changed;

	//memDp = &rmp->mp_memmap[D];	/* TODO Pointer to data segment map */
	memSp = &rmp->mp_memmap[S];	/* Pointer to stack segment map */
	//changed = 0;		/* TODO Set when either segment changed */

	if (memSp->len == 0)
	  return OK;	/* Don't bother init */

	return ENOMEM;	//TODO
}
