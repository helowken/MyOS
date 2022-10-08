


/* This program reads the nodetypes file and nodes.c.pat file. It generates
 * the files nodes.h and nodes.c.
 */

#include <stdio.h>
#include <stdlib.h>


#define MAX_TYPES	50	/* Max number of node types */
#define MAX_FIELDS	20	/* Max fields in a structure */
#define BUF_LEN		100	/* Size of charaacter buffers */

/* Field types */
#define T_NODE	1		/* Union Node *field */
#define T_NODE_LIST	2	/* NodeLsit *field */
#define T_STRING	3
#define T_INT	4		/* Int field */
#define T_OTHER	5		/* Other */
#define T_TEMP	6		/* Don't copy this field */


typedef struct {
	char *name;		/* Name of field */
	int type;		/* Type of field */
	char *decl;		/* Declaration of field */
} Field;			/* A structure field */


typedef struct {
	char *tag;		/* Structure tag */
	int numFields;	/* Number of fields in the structure */
	Field field[MAX_FIELDS];	/* The fields of the structure */
	int done;		/* Set if fully parsed */
} Struct;			/* Representing a node structure */


static int numTypes;		/*Number of node types */
static char *nodeName[MAX_TYPES];	/* Names of te nodes */
static Struct *nodeStruct[MAX_TYPES];	/* Type of structure used by the node */
static int numStruct;		/* Number of structures */
static Struct structs[MAX_TYPES];	/* The structures */
static Struct *currStruct;	/* Current structure */


static FILE *inFp = stdin;
static char line[1024];
static int lineNum;
static char *linePtr;

#define equal(s1, s2)	(strcmp(s1, s2) == 0)

static int readLine() {
	register char *p;

	if (fgets(line, 1024, inFp) == NULL)
	  return 0;
	for (p = line; *p != '#' && *p != '\n' && *p != '\0'; ++p) {
	}
	while (p > line && (p[-1] == ' ' || p[-1] == '\t')) {
		--p;
	}
	*p = '\0';
	linePtr = line;
	++lineNum;
	if (p - line > BUF_LEN)
	  error("Line too long");
	return 1;
}

int nextField(char *buf) {
	register char *p, *q;

	p = linePtr;
	while (*p == ' ' || *p == '\t') {
		++p;
	}
	q = buf;
	while (*p != ' ' && *p != '\t' && *p != '\0') {
		*q++ = *p++;
	}
	*q = '\0';
	linePtr = p;
	return q > buf;
}

static void parseField() {
	char name[BUF_LEN];
	char type[BUF_LEN];
	char decl[2 * BUF_LEN];
	Field *fp;

	if (currStruct == NULL || currStruct->done)
	  error("No current structure to add field to");
	if (! nextField(name))
	  error("No field name");
	if (! nextField(type))
	  error("No field type");
	fp = &currStruct->field[currStruct->numFields];

	//TODO
}

static void parseNode() {
	char name[BUF_LEN];
	char tag[BUF_LEN];
	Struct *sp;

	if (currStruct && currStruct->numFields > 0) 
	  currStruct->done = 1;
	nextField(name);
}

int main(int argc, char **argv) {
	if (argc != 3)
	  error("Usage: mknodes file outfile\n");
	if ((inFp = fopen(argv[1], "r")) == NULL)
	  error("can't open %s", argv[1]);
	while (readLine()) {
		if (line[0] == ' ' || line[0] == '\t')
		  parseField();
		else if (line[0] != '\0')
		  parseNode();
	}
	output(argv[2]);
	return 0;
}





