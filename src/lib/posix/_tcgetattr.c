#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>

int tcgetattr(int fd, struct termios *termios_p) {
	return ioctl(fd, TCGETS, termios_p);
}
