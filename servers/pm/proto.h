#include "timers.h"

typedef struct Memory Memory;

/* alloc.c */
void initMemory(Memory *chunks, phys_clicks *free);
void freeMemory(phys_clicks base, phys_clicks clicks);

/* utility.c */
void panic(char *who, char *msg, int num);
char *findParam(const char *key);
int getMemMap(int pNum, MemMap *memMap);
pid_t getFreePid();
