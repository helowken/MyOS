#include "pm.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "signal.h"
#include "string.h"
#include "mproc.h"
#include "param.h"

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
