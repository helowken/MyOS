extern int rootPid;		/* Pid of main shell */
extern int rootShell;	/* True if we aren't a child of the main shell */

void readCmdFile(char *);
void cmdLoop(int);
