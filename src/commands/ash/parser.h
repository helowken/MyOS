

/* Control characters in argument strings */
#define CTL_ESC		'\201'
#define CTL_VAR		'\202'
#define CTL_END_VAR	'\203'
#define CTL_BACK_Q	'\204'
#define	CTL_QUOTE	01		/* Ored with CTL_BACK_Q code if in quotes */

/* Variable substitution byte (follows CTL_VAR) */
#define VS_TYPE		07		/* Type of variable substitution */
#define VS_NUL		040		/* Colon--treat the empty string as unset */
#define VS_QUOTE	0100	/* Inside double quotes--suppress splitting */

/* Values of VS_TYPE field */
#define VS_NORMAL	1	/* Normal variable: $var or ${var} */
#define VS_MINUS	2	/* ${var-text} */
#define VS_PLUS		3	/* ${var+text} */
#define VS_QUESTION	4	/* ${var?message} */
#define VS_ASSIGN	5	/* ${var=text} */

/* NEOF is returned by parseCmd when it encounters an end of file. It
 * must be distinct from NULL, so we use the address of a variable that
 * happens to be handy.
 */
extern int tokenPushback;
#define NEOF	((Node *) &tokenPushback)

union Node;

union Node *parseCmd(int);
int goodName(char *);
