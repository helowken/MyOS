#include "lib.h"
#include "unistd.h"

int pause() {
	Message msg;

	return syscall(MM, PAUSE, &msg);
}
