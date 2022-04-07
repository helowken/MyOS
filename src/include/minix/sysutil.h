#ifndef _EXTRALIB_H
#define _EXTRALIB_H

/* Environment parsing return values. */
#define EP_BUF_SIZE		128		/* Local buffer for env value */

void envSetArgs(int argc, char *argv[]);
int envGetParam(char *key, char *value, int maxSize);

int printf(const char *fmt, ...);
void kputc(int c);
void report(char *who, char *msg, int num);
void panic(char *who, char *msg, int num);
clock_t getUptime(clock_t *ticks);
int tickDelay(clock_t ticks);

#endif
