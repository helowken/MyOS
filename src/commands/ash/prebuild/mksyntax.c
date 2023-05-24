#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../parser.h"

typedef struct {
	char *name;
	char *comment;
} SyntaxClass;

/* Syntax classes */
SyntaxClass synClass[] = {
	{"CWORD",		"character is nothing special"},
	{"CNL",		"newline character"},
	{"CBACK",		"a backslash character"},
	{"CS_QUOTE",	"single quote"},
	{"CD_QUOTE",	"double quote"},
	{"CEND_QUOTE",	"a terminating quote"},
	{"CB_QUOTE",	"backwards single quote"},
	{"CVAR",		"a dollar sign"},
	{"CEND_VAR",	"a '}' character"},
	{"CEOF",		"end of file"},
	{"CCTL",		"like CWORD, except it must be escaped"},
	{"CSPCL",		"these terminate a word"},
	{NULL, NULL}
};

/* Syntax classes for is_ functions. Warning: if you add new classes
 * you may have to change the definition of the is_in_name macro.
 */
SyntaxClass is_entry[] = {
	{"IS_DIGIT",	"a digit"},
	{"IS_UPPER",	"an upper case letter"},
	{"IS_LOWER",	"a lower case letter"},
	{"IS_UNDER",	"an underscore"},
	{"IS_SPECL",	"the name of a special parameter"},
	{NULL, NULL}
};

static char writer[] = "\
/*\n\
 * This file was generated by the mksyntax program.\n\
 */\n\
\n";

static FILE *cFile;
static FILE *hFile;
char *syntax[513];
int base;
int size;	/* # of values which a char variable can have */
int nBits;	/* # of bits in a character */
int digitConfig;	/* true if digits are contiguous */

/* Output character classification macros (e.g. isDigit). If digits are
 * contiguous, we can test for them quickly.
 */
char *macro[] = {
	"#define isDigit(c)\t((isType + SYN_BASE)[(int) c] & IS_DIGIT)",
	"#define isAlpha(c)\t((isType + SYN_BASE)[(int) c] & (IS_UPPER|IS_LOWER))",
	"#define isName(c)\t((isType + SYN_BASE)[(int) c] & (IS_UPPER|IS_LOWER|IS_UNDER))",
	"#define isInName(c)\t((isType + SYN_BASE)[(int) c] & (IS_UPPER|IS_LOWER|IS_UNDER|IS_DIGIT))",
	"#define isSpecial(c)\t((isType + SYN_BASE)[(int) c] & (IS_SPECL|IS_DIGIT))",
	NULL
};

static void outputTypeMacros() {
	char **pp;
	if (digitConfig) 
	  macro[0] = "#define isDigit(c)\t((unsigned)((c) - '0') <= 9)";
	for (pp = macro; *pp; ++pp) {
		fprintf(hFile, "%s\n", *pp);
	}
	if (digitConfig)
	  fputs("#define digitVal(c)\t((c) - '0')\n", hFile);
	else
	  fputs("#define digitVal(c)\t(digitValue[c])\n", hFile);
}

/* Clear the syntax table. */
static void fillTable(char *defVal) {
	int i;

	for (i = 0; i< size; ++i) {
		syntax[i] = defVal;
	}
}

/* Add entries to the syntax table.
 */
static void add(char *p, char *type) {
	while (*p) {
		syntax[*p++ + base] = type;
	}
}

/* Initialize the syntax table with default values.
 */
static void init() {
	fillTable("CWORD");
	syntax[0] = "CEOF";
	syntax[base + CTL_ESC] = "CCTL";
	syntax[base + CTL_VAR] = "CCTL";
	syntax[base + CTL_END_VAR] = "CCTL";
	syntax[base + CTL_BACK_Q] = "CCTL";
	syntax[base + CTL_BACK_Q + CTL_QUOTE] = "CCTL";
}

/* Output the syntax table.
 */
static void print(char *name) {
	int i, col;

	fprintf(hFile, "extern const char %s[];\n", name);
	fprintf(cFile, "const char %s[%d] = {\n", name, size);
	col = 0;
	for (i = 0; i < size; ++i) {
		if (i == 0) {
			fputs("      ", cFile);
		} else if ((i & 03) == 0) {
			fputs(",\n      ", cFile);
			col = 0;
		} else {
			putc(',', cFile);
			while (++col < 13 * (i & 03)) {
				putc(' ', cFile);
			}
		}
		fputs(syntax[i], cFile);
		col += strlen(syntax[i]);
	}
	fputs("\n};\n", cFile);
}

/* Output digit conversion table (if digits are not contiguous).
 */
static void digitConvert() {
	int maxDigit;
	static char digit[] = "0123456789";
	char *p;
	int i;

	maxDigit = 0;
	for (p = digit; *p; ++p) {
		if (*p > maxDigit)
		  maxDigit = *p;
	}
	fputs("extern const char digitValue[];\n", hFile);
	fputs("\n\nconst char digitValue] = {\n", cFile);
	for (i = 0; i <= maxDigit; ++i) {
		for (p = digit; *p && *p != i; ++p) {
		}
		if (*p == '\0')
		  p = digit;
		fprintf(cFile, "    %d,\n", p - digit);
	}
	fputs("};\n", cFile);
}

int main() {
	char c, d;
	int sign, i, pos;
	static char digit[] = "0123456789";
	char buf[80];

	/* Create output files */
	if ((cFile = fopen("../syntax.c", "w")) == NULL) {
		perror("syntax.c");
		exit(2);
	}
	if ((hFile = fopen("../syntax.h", "w")) == NULL) {
		perror("syntax.h");
		exit(2);
	}
	fputs(writer, hFile);
	fputs(writer, cFile);
	
	/* Determine the characteristics of chars. */
	c = -1;
	sign = c < 0 ? 1 : 0;
	for (nBits = 1; ; ++nBits) {
		d = (1 << nBits) - 1;
		if (d == c)
		  break;
	}
	printf("%s %d bit chars\n", sign ? "signed" : "unsigned", nBits);
	if (nBits > 9) {
		fputs("Characters can't have more than 9 bits\n", stderr);
		exit(2);
	}
	size = (1 << nBits) + 1;
	base = 1;
	if (sign)
	  base += 1 << (nBits - 1);
	digitConfig = 1;
	for (i = 0; i < 10; ++i) {
		if (digit[i] != '0' + i) { 
			digitConfig = 0;
			break;
		}
	}

	fputs("#include \"sys/cdefs.h\"\n", hFile);

	/* Generate the #define statements in the header file */
	fputs("/* Syntax classes */\n", hFile);
	for (i = 0; synClass[i].name; ++i) {
		sprintf(buf, "#define %s %d", synClass[i].name, i);
		fputs(buf, hFile);
		for (pos = strlen(buf); pos < 32; pos = (pos + 8) &~ 07) {
			putc('\t', hFile);
		}
		fprintf(hFile, "/* %s */\n", synClass[i].comment);
	}
	putc('\n', hFile);
	fputs("/* Syntax classes for is_ functions */\n", hFile);
	for (i = 0; is_entry[i].name; ++i) {
		sprintf(buf, "#define %s %#o", is_entry[i].name, 1 << i);
		fputs(buf, hFile);
		for (pos = strlen(buf); pos < 32; pos = (pos + 8) &~ 07) {
			putc('\t', hFile);
		}
		fprintf(hFile, "/* %s */\n", is_entry[i].comment);
	}
	putc('\n', hFile);
	fprintf(hFile, "#define SYN_BASE %d\n", base);
	fprintf(hFile, "#define PEOF %d\n\n", -base);
	putc('\n', hFile);
	fputs("#define BASE_SYNTAX (baseSyntax + SYN_BASE)\n", hFile);
	fputs("#define DQ_SYNTAX (dqSyntax + SYN_BASE)\n", hFile);
	fputs("#define SQ_SYNTAX (sqSyntax + SYN_BASE)\n", hFile);
	putc('\n', hFile);
	outputTypeMacros();		/* isDigit, etc. */
	putc('\n', hFile);

	/* Generate the syntax tables. */
	fputs("#include \"shell.h\"\n", cFile);
	fputs("#include \"syntax.h\"\n\n", cFile);

	init();
	fputs("/* Syntax table used when not in quotes */\n", cFile);
	add("\n", "CNL");
	add("\\", "CBACK");
	add("'", "CS_QUOTE");
	add("\"", "CD_QUOTE");
	add("`", "CB_QUOTE");
	add("$", "CVAR");
	add("}", "CEND_VAR");
	add("<>();&| \t", "CSPCL");
	print("baseSyntax");

	init();
	fputs("\n/* Syntax Table used when in double quotes */\n", cFile);
	add("\n", "CNL");
	add("\\", "CBACK");
	add("\"", "CEND_QUOTE");
	add("`", "CB_QUOTE");
	add("$", "CVAR");
	add("}", "CEND_VAR");
	add("!*?[=", "CCTL");
	print("dqSyntax");

	init();
	fputs("\n/* Syntax table used when in single quotes */\n", cFile);
	add("\n", "CNL");
	add("'", "CEND_QUOTE");
	add("!*?[=", "CCTL");
	print("sqSyntax");

	fillTable("0");
	fputs("\n/* Character classification table */\n", cFile);
	add("0123456789", "IS_DIGIT");
	add("abcdefghijklmnopqrstucvwxyz", "IS_LOWER");
	add("ABCDEFGHIJKLMNOPQRSTUCVWXYZ", "IS_UPPER");
	add("_", "IS_UNDER");
	add("#?$!-*@", "IS_SPECL");
	print("isType");
	if (! digitConfig)
	  digitConvert();

	return 0;
}













