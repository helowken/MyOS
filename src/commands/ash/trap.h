extern int pendingSigs;

void clearTraps();
int setSignal(int);
void ignoreSig(int);
void doTrap();
void setInteractive(int);
void exitShell(int);
