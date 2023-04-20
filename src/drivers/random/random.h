void randomInit(void);
int randomIsSeeded(void);
void randomUpdate(int source, unsigned short *buf, int count);
void randomGetBytes(void *buf, size_t size);
void randomPutBytes(void *buf, size_t size);
