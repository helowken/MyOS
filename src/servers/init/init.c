#include "minix/type.h"
#include "sys/types.h"
#include "sys/wait.h"
#include "sys/stat.h"
#include "sys/svrctl.h"
#include "ttyent.h"
#include "errno.h"
#include "fcntl.h"
#include "limits.h"
#include "signal.h"
#include "string.h"
#include "time.h"
#include "stdlib.h"
#include "unistd.h"
#include "utmp.h"

void main() {
	
	struct stat sb;

	if (fstat(0, &sb) < 0) {
		/* Open standard input, output & error. */
		open("/dev/null", O_RDONLY);
		open("/dev/log", O_WRONLY);
		dup(1);
	}
}
