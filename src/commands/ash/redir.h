


/* Flags passed to redirect */
#define REDIR_PUSH		01		/* Save previous values of file descriptors */
#define REDIR_BACK_Q	02		/* Save the command output in memory */

union Node;

void redirect(union Node *, int);
void popRedir();
int copyFd(int, int);
void clearRedir();
