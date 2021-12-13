#include "timers.h"

/* alloc.c */
void initMemory(Memory *chunks, phys_clicks *free);

/* utility.c */
void panic(char *who, char *msg, int num);
char *findParam(const char *key);
int getMemMap(int pNum, MemMap *memMap);
pid_t getFreePid();
