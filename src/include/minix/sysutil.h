#ifndef _EXTRALIB_H
#define _EXTRALIB_H

/* Environment parsing return values. */
#define EP_BUF_SIZE		128		/* Local buffer for env value */

void envSetArgs(int argc, char *argv[]);
int envGetParam(char *key, char *value, int maxSize);

#define fKeyMap(fKeys, sfKeys)	fKeyCtl(FKEY_MAP, (fKeys), (sfKeys))
#define fKeyUnmap(fKeys, sfKeys)	fKeyCtl(FKEY_UNMAP, (fKeys), (sfKeys))
#define fKeyEvents(fKeys, sfKeys)	fKeyCtl(FKEY_EVENTS, (fKeys), (sfKeys))
int fKeyCtl(int req, int *fKeys, int *sfKeys);


int printf(const char *fmt, ...);
void kputc(int c);
void report(char *who, char *msg, int num);
void panic(char *who, char *msg, int num);
clock_t getUptime(clock_t *ticks);
int tickDelay(clock_t ticks);

#endif
