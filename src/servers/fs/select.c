#include "fs.h"
#include "select.h"
#include "sys/time.h"
#include "sys/select.h"

/* Max number of simultaneously pending select() calls */
#define MAX_SELECTS	25

typedef struct {
	FProc *requestor;	/* Slot is free iff this is NULL */
	int reqProcNum; 
	Timer timer;	/* If expiry > 0 */
} SelectEntry;
static SelectEntry selectTable[MAX_SELECTS];

void initSelect() {
	int i;

	for (i = 0; i < MAX_SELECTS; ++i) {
		fsInitTimer(&selectTable[i].timer);
	}
}

int selectNotified(int major, int minor, int ops) {
//TODO
	return 0;
}
