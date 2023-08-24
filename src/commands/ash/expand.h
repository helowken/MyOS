


typedef struct StrList {
	struct StrList *next;
	char *text;
} StrList;

typedef struct {
	StrList *list;
	StrList **last;
} ArgList;

void expandArg(Node *, ArgList *, int);
void rmEscapes(char *);
int patternMatch(char *, char *);
int caseMatch(Node *, char *);
void expandHere(Node *, int);
