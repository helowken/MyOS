#include "fs.h"
#include "timers.h"
#include "minix/syslib.h"
#include "minix/com.h"

void fsInitTimer(Timer *tp) {
	timerInit(tp);
}
