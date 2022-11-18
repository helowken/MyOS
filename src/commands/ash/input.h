



/* The input line number. Input.c just defines this variable, and saves
 * and restores it when files are pushed and poped. The user of this
 * package must set its value.
 */
extern int parseLineNum;		
extern int parseNumLeft;		/* Number of characters left in input buffer */
extern char *parseNextChar;		/* Next character in input buffer */

char *pfGetStr(char *, int);
int pReadBuffer();
int pGetChar();
int pReadBuffer();
void pUngetChar();
void pPushback(char *, int);
void setInputFile(char *, int);
void setInputFd(int, int);
void setInputString(char *, int);
void popFile();
void popAllFiles();

#define pGetCharMacro()	(--parseNumLeft >= 0 ? *parseNextChar++ : pReadBuffer())

#if READLINE
/* This variable indicates the prompt to use with readline,
 * *and* that readline may only be used if non-NULL.
 */
extern char *readUsePrompt;
#endif
