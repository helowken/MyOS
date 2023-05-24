
#include "bltin.h"

#define RE_END		0	/* End of regular expression */
#define RE_LITERAL	1	/* Normal character follows */
#define RE_DOT		2	/* "." */
#define RE_CCL		3	/* "[...]" */
#define RE_NCCL		4	/* "[^...]" */
#define RE_LP		5	/* "\(" */
#define RE_RP		6	/* "\)" */
#define RE_MATCHED	7	/* "\digit" */
#define	RE_EOS		8	/* "$" matches end of string */
#define RE_STAR		9	/* "*" */
#define RE_RANGE	10	/* "\{num,num\}" */

/* Supports 9 backreference capturing groups at most. 
 * The 0th is used to store the original string.
 */
char *matchBegin[10];	
short matchLength[10];
short numberParens;

char *reCompile(char *pattern) {
	register char *p, *q;
	register char c;
	char *comp;
	char *begin, *endp;
	register int len;
	int first;
	int type;
	char *stackPtr;
	char stack[10];
	int parenNum;
	int i;
	void *malloc(size_t size);

	p = pattern;
	if (*p == '^')
	  ++p;
	/* Each symbol can be expressed with up to 2 tokens. 
	 * e.g., "RE_LITERAL c".
	 */
	comp = q = malloc(2 * strlen(p) + 1);
	begin = q;
	stackPtr = stack;
	parenNum = 0;

	for (;;) {
		switch ((c = *p++)) {
			case '\0':
				*q = '\0';
				goto out;
			case '.':
				*q++ = RE_DOT;
				len = 1;
				break;
			case '[':
				/* [a-b]:  RE_CCL - a b \0 (len = 5)
				 * [^ab-]: RE_NCCL a b - - - \0 (len = 7)
				 */
				begin = q;
				*q = RE_CCL;
				if (*p == '^') {
					*q = RE_NCCL;
					++p;
				}
				++q;
				first = 1;
				while (*p != ']' || first == 1) {
					if (p[1] == '-' && p[2] != ']') {
						*q++ = '-';
						*q++ = p[0];
						*q++ = p[2];
						p += 3;
					} else if (*p == '-') {
						*q++ = '-';
						*q++ = '-';
						*q++ = '-';
						++p;	
					} else {
						*q++ = *p++;
					}
					first = 0;
				}
				++p;
				*q++ = '\0';
				len = q - begin;	
				break;
			case '$':
				if (*p != '\0')
				  goto dft;
				*q++ = RE_EOS;
				break;
			case '*':
				if (len == 0)	/* No token to repeat */
				  goto dft;
				type = RE_STAR;
range:
				/* a\{n,m\}: (i=3)
				 *		RE_RANGE n m RE_LITERAL a
				 * 
				 * [a-bc]{n,m}: (i=3) 
				 *		RE_RANGE n m RE_CCL a - b c \0
				 *
				 * [a-z0-9]*: (i=1)
				 *		RE_STAR RE_CCL - a z - 0 9 \0
				 *
				 * .*: (i=1)    
				 *		RE_STAR RE_DOT 
				 *
				 * a*: (i=1)    
				 *		RE_STAR RE_LITERAL a
				 */
				i = (type == RE_RANGE) ? 3 : 1;
				endp = q + i;
				begin = q - len;
				do {
					--q;
					*(q + i) = *q;
				} while (--len > 0);
				q = begin;
				*q++ = type;
				if (type == RE_RANGE) {
					i = 0;
					while ((unsigned) (*p - '0') <= 9) {
						i = 10 * i + (*p++ - '0');
					}
					*q++ = i;	/* Convert int to char, low value */
					if (*p != ',') {
						*q++ = i;	/* Without high value */
					} else {
						++p;
						i = 0;
						while ((unsigned) (*p - '0') <= 9) {
							i = 10 * i + (*p++ - '0');
						}
						*q++ = i;	/* Convert int to char, high value */
					}
					if (*p != '\\' || *++p != '}')
					  error("RE error");
					++p;
				}
				q = endp;
				break;
			case '\\':
				/*
				 * \(ab\)\1: 
				 *   RE_LP 1 RE_LITERAL a RE_LITERAL b RE_RP 1 RE_MATCHED 1
				 *
				 * RE_MATCHED n means the nth capturing group.
				 */
				if ((c = *p++) == '(') {
					if (++parenNum > 9)
					  error("RE error");
					*q++ = RE_LP;
					*q++ = parenNum;
					*stackPtr++ = parenNum;
					len = 0;
				} else if (c == ')') {
					if (stackPtr == stack)
					  error("RE error");
					*q++ = RE_RP;
					*q++ = *--stackPtr;
					len = 0;
				} else if (c == '{') {
					type = RE_RANGE;
					goto range;
				} else if ((unsigned) (c - '1') < 9) {
					/* Should check validity here */
					*q++ = RE_MATCHED;
					*q++ = c - '0';
					len = 2;
				} else {
					goto dft;
				}
				break;
			default:
dft:
				/* RE_LITERAL c */
				*q++ = RE_LITERAL;
				*q++ = c;
				len = 2;
				break;

		}
	}
out:
	if (stackPtr != stack)
	  error("RE error");
	numberParens = parenNum;
	return comp;
}

static int match(char *pattern, char *string) {
	register char *p, *q;
	int counting;
	int low, high, count;
	char *currPat;
	char *startCount;
	int negate;
	int found;
	char *r;
	int len;
	char c;

	p = pattern;
	q = string;
	counting = 0;
	for (;;) {
		if (counting) {
			if (++count > high)
			  goto bad;
			p = currPat;	/* Reset p to the repeating pattern */
		}
		switch (*p++) {
			case RE_END:
				matchLength[0] = q - matchBegin[0];
				return 1;
			case RE_LITERAL:
				if (*q++ != *p++)
				  goto bad;
				break;
			case RE_DOT:
				if (*q++ == '\0')
				  goto bad;
				break;
			case RE_CCL:
				negate = 0;
				goto ccl;
			case RE_NCCL:
				negate = 1;
ccl:
				found = 0;
				c = *q++;
				while (*p) {	/* RE_CCL has a trailing \0 */
					if (*p == '-') {
						if (c >= *++p && c <= *++p)
						  found = 1;
					} else {
						if (c == *p)
						  found = 1;
					}
					++p;
				}
				++p;	/* Skip the \0 of RE_CCL */
				if (found == negate || c == 0)
				  goto bad;
				break;
			case RE_LP:	/* seq: RE_LP n, n = *p */
				/* Mark down the start of the group. */
				matchBegin[(int) *p++] = q;
				break;
			case RE_RP: /* seq: RE_RP n, n = *p */
				/* Store the length of the group. */
				matchLength[(int) *p] = q - matchBegin[(int) *p];
				++p;
				break;
			case RE_MATCHED: /* seq: RE_MATCHED n, n = *p */
				r = matchBegin[(int) *p];	/* r = the nth group */
				len = matchLength[(int) *p++]; 
				/* Check if the group is matched. */
				while (--len >= 0) {
					if (*q++ != *r++)
					  goto bad;
				}
				break;
			case RE_EOS:
				if (*q != '\0')
				  goto bad;
				break;
			case RE_STAR: /* seq: RE_STAR */
				low = 0;
				high = 32767;
				goto range;
			case RE_RANGE: /* seq: RE_RANGE n m */
				low = *p++;
				high = *p++;
				if (high == 0)	/* For: \{0\} */
				  high = 32767;
range:
				currPat = p;	/* Mark down the repeating pattern */
				startCount = q;
				count = 0;
				++counting;
				break;
		}
	}
bad:
	if (! counting)	/* not RE_RANGE, no need to look back */
	  return 0;
	
	/* RE_RANGE may be greedy, need to go back to check again.
	 * e.g.,
	 *	string: "aab"
	 *	regexp: "a*b"
	 * when go here, it means RE_RANGE expects "aaa", but failed,
	 * so need to go back and check the last character and will 
	 * be success finally.
	 */
	len = 1;
	if (*currPat == RE_MATCHED)
	  len = matchLength[(int) currPat[1]];
	while (--count >= low) {
		if (match(p, startCount + count * len))
		  return 1;
	}
	return 0;
}

int reMatch(char *pattern, char *string) {
	char **pp;

	matchBegin[0] = string;
	for (pp = &matchBegin[1]; pp <= &matchBegin[9]; ++pp) {
		*pp = 0;
	}
	return match(pattern, string);
}


