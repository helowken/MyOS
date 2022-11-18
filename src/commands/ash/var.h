

/* Shell variables.
 */

/* Flags */
#define V_EXPORT		01	/* Variable is exported */
#define V_READONLY		02	/* Variable cannot be modified */
#define V_STR_FIXED		04	/* Variable struct is staticly allocated */
#define V_TEXT_FIXED	010	/* Text is staticly allocated */
#define V_STACK			020	/* Text is allocated on the stack */
#define V_UNSET			040	/* The variable is not set */

typedef struct Var {
	struct Var *next;	/* Next entry in hash list */
	int flags;		/* Flags are defined aboved */
	char *text;		/* Name=value */
} Var;

typedef struct LocalVar {
	struct LocalVar *next;	/* Next local variable in list */
	struct Var *vp;	/* The variable that was made local */
	int flags;		/* Saved flags */
	char *text;		/* Saved text */
} LocalVar;

LocalVar *localVars;

#if ATTY
extern Var vatty;
#endif
extern Var vifs;
extern Var vmail;
extern Var vmpath;
extern Var vpath;
extern Var vps1;
extern Var vps2;
extern Var vpse;
#if ATTY
extern Var vterm;
#endif


/* The following macros access the values of the above variables.
 * They have to skip over the name. They return the null string
 * for unset variables.
 */

#define ifsVal()	(vifs.text + 4)
#define mailVal()	(vmail.text + 5)
#define mpathVal()	(vmpath.text + 9)
#define pathVal()	(vpath.text + 5)
#define ps1Val()	(vps1.text + 4)
#define ps2Val()	(vps2.text + 4)
#define pseVal()	(vpse.text + 4)
#if ATTY
#define termVal()	(vterm.text + 5)
#define attySet()	((vatty.flags & V_UNSET) == 0)
#endif
#define mpathSet()	((vmpath.flags & V_UNSET) == 0)




void initVar();
void setVar(char *, char *, int);
void setVarEq(char *, int);
char *lookupVar(char *);





