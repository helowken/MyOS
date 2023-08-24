/* dmp.c */
int doFKeyPressed(Message *msg);
void mappingDmp(void);

/* dmp_kernel.c */
void procTabDmp(void);
void memmapDmp(void);
void privilegesDmp(void);
void imageDmp(void);
void irqTabDmp(void);
void kernelMsgDmp(void);
void scheduleDmp(void);
void monParamsDmp(void);
void kernelEnvDmp(void);
void timingDmp(void);

/* dmp_pm.c */
void mprocDmp(void);
void sigactionDmp(void);

/* dmp_fs.c */
void devTabDmp(void);
void fprocDmp(void);

