#include <lib.h>
#include <minix/minlib.h>

static int next;
static char qBuf[8];

char *itoa(int n) {
	register int r, k;
	int flag = 0;

	next = 0;
	if (n < 0) {
		qBuf[next++] = '-';
		n = -n;
	}
	if (n == 0) {
		qBuf[next++] = '0';
	} else {
		k = 10000;
		while (k > 0) {
			r = n / k;
			if (flag || r > 0) {
				qBuf[next++] = '0' + r;
				flag = 1;
			}
			n -= r * k;
			k = k / 10;
		}
	}
	qBuf[next] = 0;
	return qBuf;
}
