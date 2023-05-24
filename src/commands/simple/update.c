#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

int main() {
	/* Release all (?) open file descriptiors. */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Release current directory to avoid locking current device. */
	chdir("/");

	/* Flush the cache every 30 seconds. */
	while (1) {
		sync();
		sleep(30); 
	}
}
