#ifndef _EXTRALIB_H
#define _EXTRALIB_H

int printf(const char *fmt, ...);
void kputc(int c);
void panic(char *who, char *msg, int num);
clock_t getUptime(clock_t *ticks);

#endif
