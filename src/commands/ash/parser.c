


#include "shell.h"
#include "parser.h"
#include "nodes.h"
#include "expand.h"
#include "syntax.h"
#include "options.h"
#include "input.h"
#include "var.h"
#include "output.h"
#include "error.h"
#include "memalloc.h"
#include "mystring.h"
#include "token.h"

#define EOF_MARK_LEN	79


typedef struct Heredoc {
	struct Heredoc *next;		/* Next here document in list */
	Node *here;					/* Redirection node */
	char *eofMark;				/* String indicating end of input */
	int stripTabs;				/* If set, strip leading tabs */
} Heredoc;


static Heredoc *heredocList;	/* List of here documents to read */
static int parseBackquote;		/* Nonzero if we are inside backquotes */
static int doPrompt;			/* If set, prompt the user */
static int needPrompt;			/* True if interactive and at start of line */
static int lastToken;			/* Last token read */
MKINIT int tokenPushback;		/* Last token pushed back */
static char *wordText;			/* Text of last word returned by readToken */
static int checkKwd;			/* 1 = check for kwds, 2 = also eat newlines */
static NodeList *backquoteList;	
Node *redirNode;
Heredoc *heredoc;
static int quoteFlag;			/* Set if (part of) last token was quoted */
static int startLineNum;		/* Line # where last token started */

static const char argVars[5] = {CTL_VAR, VS_NORMAL | VS_QUOTE, '@', '=', '\0'};
static const char types[] = "}-+?=";

static int readToken();
static void parseHeredoc();
static Node *list(int nlFlag);
static Node *command();


#if READLINE
static void putPrompt(char *s) {
	if (editable) {
		readUsePrompt = s;
	} else {
		out2Str(s);
	}
}
#else
#define putPrompt(s)	out2Str(s)
#endif

static void syntaxError(char *msg) {
	if (commandName) 
	  outFormat(&errOut, "%s: %d: ", commandName, startLineNum);
	outFormat(&errOut, "Syntax error: %s\n", msg);
	error((char *) NULL);
}

/* Called when an unexpected token is read during the parse. The argument
 * is the token that is expected, or -1 if more than one type of token can
 * occur at this point.
 */
static void syntaxExpect(int token) {
	char msg[64];

	if (token >= 0) 
	  formatStr(msg, 64, "%s unexpected (expecting %s)", 
				  tokenName[lastToken], tokenName[token]);
	else
	  formatStr(msg, 64, "%s unexpected", tokenName[lastToken]);
	syntaxError(msg);
}

static int peekToken() {
	int t;

	t = readToken();
	++tokenPushback;
	return t;
}

/* Returns true if the text contains nothing to expand (no dollar signs
 * or backquotes).
 */
static int noExpand(char *text) {
	register char *p;
	register int c;

	p = text;
	while ((c = *p++) != '\0') {
		if (c == CTL_ESC)
		  ++p;
		else if (BASE_SYNTAX[c] == CCTL)
		  return 0;
	}
	return 1;
}

/* Parse the right part. 
 */
static void parseFileName() {
	Node *n = redirNode;

	if (readToken() != TWORD) 
	  syntaxExpect(-1);
	if (n->type == NHERE) {
		Heredoc *here = heredoc;
		Heredoc *p;
		int i;

		if (quoteFlag == 0)
		  n->type = NXHERE;
		if (here->stripTabs) {
			while (*wordText == '\t') {
				++wordText;
			}
		}
		if (! noExpand(wordText) || (i = strlen(wordText)) == 0 || i > EOF_MARK_LEN)
		  syntaxError("Illegal eof marker for << redirection");
		rmEscapes(wordText);
		here->eofMark = wordText;
		here->next = NULL;
		if (heredocList == NULL)
		  heredocList = here;
		else {
			for (p = heredocList; p->next; p = p->next) {
			}
			p->next = here;
		}
	} else if (n->type == NTOFD || n->type == NFROMFD) {
		/* NFile contains NDup */
		if (isDigit(wordText[0]))
		  n->nDup.dupFd = digitVal(wordText[0]);
		else if (wordText[0] == '-')
		  n->nDup.dupFd = -1;
		else
		  goto bad;
		if (wordText[1] != '\0') {
bad:
			syntaxError("Bad fd number");
		}
	} else {
		n->nFile.fileName = (Node *) stackAlloc(sizeof(NArg));
		n = n->nFile.fileName;
		n->type = NARG;
		n->nArg.next = NULL;
		n->nArg.text = wordText;
		n->nArg.backquote = backquoteList;
	}
}

int goodName(char *name) {
	register char *p;

	p = name;
	if (! isName(*p))
	  return 0;
	while (*++p) {
		if (! isInName(*p))
		  return 0;
	}
	return 1;
}

static Node *simpleCmd(Node **rpp, Node *redir) {
	Node *args, **app;
	Node **origRpp;
	Node *n;

	/* If we don't have any redirections already, then we must reset
	 * rpp to be the address of the local redir variable. */
	if (redir == 0)
	  rpp = &redir;

	args = NULL;
	app = &args;
	/* We save the incoming value, because we need this for shell
	 * functions. There can not be a redirect or an argument between
	 * the function name and the open parenthesis.
	 */
	origRpp = rpp;
	for (;;) {
		if (readToken() == TWORD) {
			n = (Node *) stackAlloc(sizeof(NArg));
			n->type = NARG;
			n->nArg.text = wordText;
			n->nArg.backquote = backquoteList;
			*app = n;
			app = &n->nArg.next;
		} else if (lastToken == TREDIR) {
			*rpp = n = redirNode;
			rpp = &n->nFile.next;
			parseFileName();	/* Read name of redirection file */
		} else if (lastToken == TLP && 
					app == &args->nArg.next &&
					rpp == origRpp) {
			/* We have a function */
			if (readToken() != TRP)
			  syntaxExpect(TRP);
			n->type = NDEFUN;
			n->nArg.next = command();
			return n;
		} else {
			++tokenPushback;
			break;
		}
	}
	*app = NULL;
	*rpp = NULL;
	n = (Node *) stackAlloc(sizeof(NCmd));
	n->type = NCMD;
	n->nCmd.backgnd = 0;
	n->nCmd.args = args;
	n->nCmd.redirect = redir;

	return n;
}

static Node *command() {
	Node *n1, *n2;
	Node *ap, **app;
	Node *cp, **cpp;
	Node *redir, **rpp;
	int t;

	checkKwd = 2;
	redir = 0;	
	rpp = &redir;
	/* Check for redirection which may precede command. */
	while (readToken() == TREDIR) {
		*rpp = n2 = redirNode;
		rpp = &n2->nFile.next;
		parseFileName();
	}
	++tokenPushback;

	switch (readToken()) {
		case TIF:
			/* Simple structure:
			 *
			 * if command
			 * then 
			 *    commands
			 * else
			 *    commands
			 * fi
			 * 
			 * =====================
			 * Complex structure:
			 *
			 * if command1
			 * then
			 *    commands1
			 * elif command2
			 * then
			 *    commands2
			 * else
			 *    commands3
			 * fi
			 *
			 * When parsing, we convert the complex structure to:
			 *
			 * if command1
			 * then
			 *    commands1
			 * else
			 *   if command2
			 *   then
			 *      commands2
			 *   else
			 *      commands3
			 *   fi
			 * fi
			 */
			n1 = (Node *) stackAlloc(sizeof(NIf));
			n1->type = NIF;
			n1->nIf.test = list(0);		/* if command1 */
			if (readToken() != TTHEN)
			  syntaxExpect(TTHEN);
			n1->nIf.ifPart = list(0);	/* then commands1 */
			n2 = n1;
			while (readToken() == TELIF) {
				n2->nIf.elsePart = (Node *) stackAlloc(sizeof(NIf));
				n2 = n2->nIf.elsePart;
				n2->type = NIF;
				n2->nIf.test = list(0);	/* if command2 */
				if (readToken() != TTHEN)
				  syntaxExpect(TTHEN);
				n2->nIf.ifPart = list(0);	/* then commands2 */
			}
			if (lastToken == TELSE)
			  n2->nIf.elsePart = list(0);	/* else commands3 */
			else {
				n2->nIf.elsePart = NULL;
				++tokenPushback;
			}
			if (readToken() != TFI)
			  syntaxExpect(TFI);
			checkKwd = 1;
			break;
		case TWHILE:
		case TUNTIL: {
			/* (while | until) testCommands
			 * do
			 *    otherCommands
			 * done
			 */
			int got;
			n1 = (Node *) stackAlloc(sizeof(NBinary));
			n1->type = lastToken == TWHILE ? NWHILE : NUNTIL;
			n1->nBinary.ch1 = list(0);	/* testCommands */
			if ((got = readToken()) != TDO) 
			  syntaxExpect(TDO);
			n1->nBinary.ch2 = list(0);	/* otherCommands */
			if (readToken() != TDONE)
			  syntaxExpect(TDONE);
			checkKwd = 1;
			break;
		}
		case TFOR:
			/* for variable in (list | $list | command)
			 * do
			 *    commands
			 * done
			 */
			if (readToken() != TWORD || quoteFlag || ! goodName(wordText))
			  syntaxError("Bad for loop variable");
			n1 = (Node *) stackAlloc(sizeof(NFor));
			n1->type = NFOR;
			n1->nFor.var = wordText;
			if (readToken() == TWORD && ! quoteFlag && equal(wordText, "in")) {
				/* in list */
				app = &ap;
				while (readToken() == TWORD) {	
					n2 = (Node *) stackAlloc(sizeof(NArg));
					n2->type = NARG;
					n2->nArg.backquote = backquoteList;
					*app = n2;
					app = &n2->nArg.next;
				}
				*app = NULL;
				n1->nFor.args = ap;
				/* A newline or semicolon is required here to end the list */
				if (lastToken != TNL && lastToken != TSEMI)
				  syntaxExpect(-1);
			} else {
				n2 = (Node*) stackAlloc(sizeof(NArg));
				n2->type = NARG;
				n2->nArg.text = (char *) argVars;
				n2->nArg.backquote = NULL;
				n2->nArg.next = NULL;
				n1->nFor.args = n2;
				/* A newline or semicolon is optional here. Anything else gets
				 * pushed back so we can read it again. */
				if (lastToken != TNL && lastToken != TSEMI)
				  ++tokenPushback;
			}
			checkKwd = 2;
			if ((t = readToken()) == TDO)
			  t = TDONE;
			else if (t == TBEGIN)
			  t = TEND;
			else
			  syntaxExpect(-1);
			n1->nFor.body = list(0);	/* commands */
			if (readToken() != t)
			  syntaxExpect(t);
			checkKwd = 1;
			break;
		case TCASE:
			/* case variable in
			 * pattern1 | pattern2) commands1;;
			 * pattern3) commands2;;
			 * esac
			 */
			n1 = (Node *) stackAlloc(sizeof(NCase));	
			n1->type = NCASE;
			if (readToken() != TWORD)
			  syntaxExpect(TWORD);
			/* case variable in */
			n1->nCase.expr = n2 = (Node *) stackAlloc(sizeof(NArg));
			n2->type = NARG;
			n2->nArg.text = wordText;
			n2->nArg.backquote = backquoteList;
			n2->nArg.next = NULL;
			while (readToken() == TNL) {
			}
			if (lastToken != TWORD || ! equal(wordText, "in"))
			  syntaxError("expecting \"in\"");
			cpp = &n1->nCase.cases;
			while (checkKwd = 2, readToken() == TWORD) {
				*cpp = cp = (Node *) stackAlloc(sizeof(NCaseList));
				/* pattern1 | pattern2) commands1;;
				 * or 
				 * pattern3) commands2;;
				 */
				cp->type = NCASELIST;
				app = &cp->nCaseList.pattern;
				for (;;) {
					*app = ap = (Node *) stackAlloc(sizeof(NArg));
					ap->type = NARG;
					ap->nArg.text = wordText;
					ap->nArg.backquote = backquoteList;
					if (readToken() != TPIPE)	/* pattern3 */
					  break;
					/* else for: pattern1 | pattern2 */
					app = &ap->nArg.next;
					if (readToken() != TWORD)
					  syntaxExpect(TWORD);
				}
				ap->nArg.next = NULL;
				if (lastToken != TRP)	/* ')' */
				  syntaxExpect(TRP);
				cp->nCaseList.body = list(0);	/* commands */
				if ((t = readToken()) == TESAC)
				  ++tokenPushback;
				else if (t != TEND_CASE)	/* expect ';;' to end the case */
				  syntaxExpect(TEND_CASE);
				cpp = &cp->nCaseList.next;
			}
			*cpp = NULL;
			if (lastToken != TESAC)
			  syntaxExpect(TESAC);
			checkKwd = 1;
			break;
		case TLP:	
			/* (commands) 
			 * 
			 * executes in a subshell
			 */
			n1 = (Node *) stackAlloc(sizeof(NRedir));
			n1->type = NSUBSHELL;
			n1->nRedir.n = list(0);
			n1->nRedir.redirect = NULL;
			if (readToken() != TRP)
			  syntaxExpect(TRP);
			checkKwd = 1;
			break;
		case TBEGIN:
			/* { commands } */
			n1 = list(0);
			if (readToken() != TEND)
			  syntaxExpect(TEND);
			checkKwd = 1;
			break;
		/* Handle an empty command like other simple commands. */
		case TNL:
		case TSEMI:
		case TEND:
		case TRP:
		case TEOF:
		case TWORD:
			++tokenPushback;
			return simpleCmd(rpp, redir);
		default:
			syntaxExpect(-1);
	}

	/* Now check for redirection which may follow command */
	while (readToken() == TREDIR) {
		*rpp = n2 = redirNode;
		rpp = &n2->nFile.next;
		parseFileName();
	}
	++tokenPushback;
	*rpp = NULL;
	if (redir) {	
		if (n1->type != NSUBSHELL) {
			n2 = (Node *) stackAlloc(sizeof(NRedir));
			n2->type = NREDIR;
			n2->nRedir.n = n1;
			n1 = n2;
		}
		n1->nRedir.redirect = redir;
	}
	return n1;
}

/*
 * cmd1 | cmd2 | cmd3 => return a pipeline which cmdList contains (cmd1, cmd2, cmd3)
 *
 * cmd1 => just return cmd1
 */
static Node *pipeline() {
	Node *n1, *pipeNode;
	NodeList *lp, *prev;

	n1 = command();
	if (readToken() == TPIPE) {
		pipeNode = (Node *) stackAlloc(sizeof(NPipe));
		pipeNode->type = NPIPE;
		pipeNode->nPipe.backgnd = 0;
		lp = (NodeList *) stackAlloc(sizeof(NodeList));
		pipeNode->nPipe.cmdList = lp;
		lp->node = n1;
		do {
			prev = lp;
			lp = (NodeList *) stackAlloc(sizeof(NodeList));
			lp->node = command();
			prev->next = lp;
		} while (readToken() == TPIPE);
		lp->next = NULL;
		n1 = pipeNode;
	}
	++tokenPushback;
	return n1;
}

/*
 * n1 && n2 || n5 => return the following tree structure
 *		 or	
 *      /  \
 *    and   n5
 *   /   \
 *  n1    n2
 * 
 * n1 => just return n1
 */
static Node *andOr() {
	Node *n1, *n2, *n3;
	int t;

	n1 = pipeline();
	for (;;) {
		if ((t = readToken()) == TAND) {
			t = NAND;
		} else if (t == TOR) {
			t = NOR;
		} else {
			++tokenPushback;
			return n1;
		} 
		n2 = pipeline();
		n3 = (Node *) stackAlloc(sizeof(NBinary));
		n3->type = t;
		n3->nBinary.ch1 = n1;
		n3->nBinary.ch2 = n2;
		n1 = n3;
	}
}

/* Link commands together.
 */
static Node *list(int nlFlag) {
	Node *n1, *n2, *n3, **pn;
	int first;

	checkKwd = 2;
	if (nlFlag == 0 && tokenEndList[peekToken()]) 
	  return NULL;
	n1 = andOr();
	for (first = 1; ; first = 0) {
		switch (readToken()) {
			case TBACKGND:
				pn = &n1;
				if (!first && n1->type == NSEMI)
				  pn = &n1->nBinary.ch2;
				if ((*pn)->type == NCMD || (*pn)->type == NPIPE) {
					(*pn)->nCmd.backgnd = 1;
				} else if ((*pn)->type == NREDIR) {
					(*pn)->type = NBACKGND;
				} else {
					n3 = (Node *) stackAlloc(sizeof(NRedir));
					n3->type = NBACKGND;
					n3->nRedir.n = *pn;
					n3->nRedir.redirect = NULL;
					*pn = n3;
				}
				goto tsemi;
			case TNL:
				++tokenPushback;
				/* Fall through */
tsemi:		case TSEMI:
				if (readToken() == TNL) {
					parseHeredoc();
					if (nlFlag) 
					  return n1;
				} else {
					++tokenPushback;
				}
				/* For example: 
				 *  echo aaa; echo bbb 
				 * or
				 *  echo aaa
				 *  echo bbb
				 *  (will be converted to => echo aaa; echo bbb)
				 */
				checkKwd = 2;
				if (tokenEndList[peekToken()])  
				  return n1;
				n2 = andOr();
				n3 = (Node *) stackAlloc(sizeof(NBinary));
				n3->type = NSEMI;
				n3->nBinary.ch1 = n1;
				n3->nBinary.ch2 = n2;
				n1 = n3;
				break;
			case TEOF:
				if (heredocList)
				  parseHeredoc();
				else
				  pUngetChar();		/* Push back EOF on input */
				return n1;
			default:
				if (nlFlag)
				  syntaxExpect(-1);
				++tokenPushback;
				return n1;
		}
	}
}


#define RETURN(token)	return lastToken = token

/* If eofMark is NULL, read a word or a redirection symbol. If eofMark
 * is not NULL, read a here document. In the latter case, eofMark is the
 * word which marks the end of the document and stripTabs is true if
 * leading tabs should be stripped from the document. The argument firstChar
 * is the first character of the input token or document.
 *
 * Because C does not have internal subroutines, I have simulated them
 * using goto's to implement the subroutine linkage. The following macros
 * will run code that appears at the end of readToken1.
 */

#define CHECK_END()		{goto checkEnd; checkEndReturn:;}
#define PARSE_REDIR()	{goto parseRedir; parseRedirReturn:;}
#define PARSE_SUB()		{goto parseSub; parseSubReturn:;}	
#define PARSE_BACKQUOTE_OLD()	{oldStyle = 1; goto parseBackq; parseBackqOldReturn:;}
#define PARSE_BACKQUOTE_NEW()	{oldStyle = 0; goto parseBackq; parseBackqNewReturn:;}

static int readToken1(int firstChar, char const *syntax, 
			char *eofMark, int stripTabs) {
	register int c = firstChar;
	register char *out;
	int len;
	char line[EOF_MARK_LEN + 1];
	NodeList *bqList;
	int dblquote;
	int quoteF;
	int varNest;
	int oldStyle;

	startLineNum = parseLineNum;
	dblquote = 0;
	if (syntax == DQ_SYNTAX)
	  dblquote = 1;
	quoteF = 0;
	bqList = NULL;
	varNest = 0;
	START_STACK_STR(out);
	loop: {	/* For each line, until end of word */
		  CHECK_END();	/* Set c to PEOF if at end of here document */
		  for (;;) {	/* Until end of line or end of word */
			CHECK_STR_SPACE(3, out);	/* Permit 3 calls to UST_PUTC */
			printf("'%c'", c);
			switch (syntax[c]) {
				case CNL:	/* '\n' */
					printf("1 ");
					if (syntax == BASE_SYNTAX)
					  goto endWord;		/* Exit outer loop */
					UST_PUTC(c, out);
					++parseLineNum;
					if (doPrompt)
					  putPrompt(ps2Val());
					c = pGetChar();
					goto loop;		/* Continue outer loop */
				case CWORD:
					printf("2 ");
					UST_PUTC(c, out);
					break;
				case CCTL:
					printf("3 ");
					if (eofMark == NULL || dblquote)
					  UST_PUTC(CTL_ESC, out);
					UST_PUTC(c, out);
					break;
				case CBACK:	/* Backslash */
					printf("4");
					c = pGetChar();
					if (c == PEOF) {
						printf("1 ");
						UST_PUTC('\\', out);
						pUngetChar();
					} else if (c == '\n') {
						printf("2 ");
						if (doPrompt)
						  putPrompt(ps2Val());
					} else {
						if (dblquote && c != '\\' && c != '`' && c != '$' &&
									(c != '"' || eofMark != NULL)) {
							printf("3");
						  UST_PUTC('\\', out);
						}
						if (SQ_SYNTAX[c] == CCTL) {
							printf("4");
						  UST_PUTC(CTL_ESC, out);
						}
						printf("5 ");
						UST_PUTC(c, out);
						++quoteF;
					}
					break;
				case CS_QUOTE:
					printf("5 ");
					syntax = SQ_SYNTAX;
					break;
				case CD_QUOTE:
					printf("6 ");
					syntax = DQ_SYNTAX;
					dblquote = 1;
					break;
				case CEND_QUOTE:
					printf("7 ");
					if (eofMark) {
						UST_PUTC(c, out);
					} else {
						syntax = BASE_SYNTAX;
						++quoteF;
						dblquote = 0;
					}
					break;
				case CVAR:	/* '$' */
					printf("8 ");
					PARSE_SUB();	/* Parse substitution */
					break;
				case CEND_VAR:	/* '}' */
					printf("9 ");
					if (varNest > 0) {
						--varNest;
						UST_PUTC(CTL_END_VAR, out);
					} else {
						UST_PUTC(c, out);
					}
					break;
				case CB_QUOTE:	/* '`' */
					printf("0 ");
					PARSE_BACKQUOTE_OLD();
					break;
				case CEOF:
					printf("+ ");
					goto endWord;	/* Exit outer loop */
				default:
					printf("- ");
					if (varNest == 0)
					  goto endWord;		/* Exit outer loop */
					UST_PUTC(c, out);
			}
			c = pGetCharMacro();
		  }
	}
endWord:
		  printf("\n");
	if (syntax != BASE_SYNTAX && ! parseBackquote && eofMark == NULL) 
	  syntaxError("Unterminated quoted string");
	if (varNest != 0) {
		startLineNum = parseLineNum;
		syntaxError("Missing '}'");
	}
	UST_PUTC('\0', out);
	len = out - stackBlock();
	out = stackBlock();
	if (eofMark == NULL) {
		/* Parse the left part:
		 *  >&2:  *out='\0', len = 1
		 *  1>&2: *out="1", len = 2	('1' + '\0')
		 */
		if ((c == '>' || c == '<') &&
				quoteF == 0 &&
				len <= 2 &&		
				(*out == '\0' || isDigit(*out))) {	
			PARSE_REDIR();
			RETURN(TREDIR);
		} else {
			pUngetChar();
		}
	}
	quoteFlag = quoteF;
	backquoteList = bqList;
	grabStackBlock(len);
	wordText = out;
	RETURN(TWORD);
/* End of readToken1 routine */


/* Check to see whether we are at the end of the here document. When this
 * is called, c is set to the first character of the next input line. If
 * we are at the end of the here document, this routine sets the c to PEOF.
 */
checkEnd: {
	if (eofMark) {
		if (stripTabs) {
			while (c == '\t') { 
				c = pGetChar();
			}
			if (c == *eofMark) {
				if (pfGetStr(line, sizeof(line)) != NULL) {
					register char *p, *q;

					p = line;
					for (q = eofMark + 1; *q && *p == *q; ++p, ++q) {
					}
					if (*p == '\n' && *q == '\0') {
						c = PEOF;
						++parseLineNum;
						needPrompt = doPrompt;
					} else {
						pPushback(line, strlen(line));
					}
				}
			}
		}
	}
	goto checkEndReturn;
}

/* Parse a redirection operator. The variable "out" points to a string
 * specifying the fd to be redirected. The variable "c" contains the
 * first character of the redirection operator.
 */
parseRedir: {
	char fd = *out;
	Node *np;

	np = (Node *) stackAlloc(sizeof(NFile));
	if (c == '>') {
		np->nFile.fd = 1;
		c = pGetChar();
		if (c == '>')
		  np->type = NAPPEND;
		else if (c == '&')
		  np->type = NTOFD;
		else {
			np->type = NTO;
			pUngetChar();
		}
	} else {	/* c == '<' */
		np->nFile.fd = 0;
		c = pGetChar();
		if (c == '<') {
			if (sizeof(NFile) != sizeof(NHere)) {
				np = (Node *) stackAlloc(sizeof(NHere));
				np->nFile.fd = 0;
			}
			np->type = NHERE;
			heredoc = (Heredoc *) stackAlloc(sizeof(Heredoc));
			heredoc->here = np;
			if ((c = pGetChar()) == '-') {
				heredoc->stripTabs = 1;
			} else {
				heredoc->stripTabs = 0;
				pUngetChar();
			}
		} else if (c == '&') {
			np->type = NFROMFD;
		} else {
			np->type = NFROM;
			pUngetChar();
		}
	}
	/* >&2:  fd = '\0'
	 * 1>&2: fd = 1
	 */
	if (fd != '\0') 
	  np->nFile.fd = digitVal(fd);
	redirNode = np;
	goto parseRedirReturn;
}

/* Parse a substitution. At this point, we have read the dollar sign
 * and nothing else.
 */
parseSub: {
	int subType;
	int typeLoc;
	int flags;
	char *p;

	c = pGetChar();
	if (c == '1') {
		printf("=== test: %d, %d\n", isName(c), isSpecial(c));
	}
	if (c != '(' && c != '{' && !isName(c) && !isSpecial(c)) {
		UST_PUTC('$', out);
		pUngetChar();
	} else if (c == '(') {	/* $(command) */
		PARSE_BACKQUOTE_NEW();
	} else {
		UST_PUTC(CTL_VAR, out);
		typeLoc = out - stackBlock();
		UST_PUTC(VS_NORMAL, out);
		subType = VS_NORMAL;
		if (c == '{') {
			c = pGetChar();
			subType = 0;
		}
		if (isName(c)) {
			do {
				ST_PUTC(c, out);
				c = pGetChar();
			} while (isInName(c));
		} else {
			if (! isSpecial(c)) 
badSub:		  syntaxError("Bad substitution");
			UST_PUTC(c, out);
			c = pGetChar();
		}
		ST_PUTC('=', out);
		flags = 0;
		if (subType == 0) {
			if (c == ':') {
				flags = VS_NUL;
				c = pGetChar();
			}
			p = strchr(types, c);
			if (p == NULL)
			  goto badSub;
			subType = p - types + VS_NORMAL;
		} else {
			pUngetChar();
		}
		if (dblquote)
		  flags |= VS_QUOTE;
		*(stackBlock() + typeLoc) = subType | flags;
		if (subType != VS_NORMAL)
		  ++varNest;
	}
	goto parseSubReturn;
}

/* Called to parse command substitutions. Newstyle is set if the command
 * is enclosed inside $(...); nlpp is a pointer to the head of the linked
 * list of commands (passed b reference), and saveLen is the number of
 * characters on the top of the stack which must be preserved.
 */
parseBackq: {
	NodeList **nlpp;
	int savePbq;
	Node *n;
	char *volatile str;
	JmpLoc jmpLoc;
	JmpLoc *volatile saveHandler;
	int saveLen;
	int saveDoPrompt;
	
	savePbq = parseBackquote;
	if (setjmp(jmpLoc.loc)) {
		if (str)
		  ckFree(str);
		parseBackquote = 0;
		handler = saveHandler;
		longjmp(handler->loc, 1);
	}
	INTOFF;
	str = NULL;
	saveLen = out - stackBlock();
	if (saveLen > 0) {
		str = ckMalloc(saveLen);
		bcopy(stackBlock(), str, saveLen);
	}
	saveHandler = handler;
	handler = &jmpLoc;
	INTON;
	if (oldStyle) {
		/* We must read until the closing backquote, giving special
		 * treatment to some slashes, and then push the string and
		 * reread it as input, interpreting it normally. */
		register char *out;
		int saveLen;
		char *str;

		START_STACK_STR(out);
		while ((c = pGetChar()) != '`') {
			if (c == '\\') {
				c = pGetChar();
				if (c != '\\' && c != '`' && c != '$' &&
							(!dblquote || c != '"'))
				  ST_PUTC('\\', out);
			}
			if (c == '\n') {
				++parseLineNum;
				if (doPrompt)
				  putPrompt(ps2Val());
			}
			ST_PUTC(c, out);
		}
		ST_PUTC('\0', out);
		saveLen = out - stackBlock();
		if (saveLen > 0) {
			str = ckMalloc(saveLen);
			bcopy(stackBlock(), str, saveLen);
		}
		setInputString(str, 1);
		saveDoPrompt = doPrompt;
		doPrompt = 0;	/* No prompts while rereading string */
	}
	nlpp = &bqList;
	while (*nlpp) {
		nlpp = &(*nlpp)->next;
	}
	*nlpp = (NodeList *) stackAlloc(sizeof(NodeList));
	(*nlpp)->next = NULL;
	parseBackquote = oldStyle;
	n = list(0);
	if (!oldStyle && (readToken() != TRP))
	  syntaxExpect(TRP);
	(*nlpp)->node = n;
	/* Start reading from old file again. */
	if (oldStyle) {
		popFile();
		doPrompt = saveDoPrompt;
	}
	while (stackBlockSize() <= saveLen) {
		growStackBlock();
	}
	START_STACK_STR(out);
	if (str) {
		bcopy(str, out, saveLen);
		ST_ADJUST(saveLen, out);
		INTOFF;
		ckFree(str);
		str = NULL;
		INTON;
	}
	parseBackquote = savePbq;
	handler = saveHandler;
	UST_PUTC(CTL_BACK_Q + dblquote, out);
	if (oldStyle)
	  goto parseBackqOldReturn;
	else
	  goto parseBackqNewReturn;
}

/* === end of readToken1 === */
}



/* Read the next input token.
 * If the token is a word, we set backquoteList to the list of cmds in
 *  backquotes. We set quoteFlag to true if any part of the word was
 *  quoted.
 * If the token is TREDIR, then we set redirNode to a strtucture containing
 *  the redirection.
 * In all cases, the variable startLineNum is set to the number of the line
 *  on which the token starts.
 */

static int xxReadToken() {
	register int c;

	if (tokenPushback) {
		tokenPushback = 0;
		return lastToken;
	}
	if (needPrompt) {
		putPrompt(ps2Val());
		needPrompt = 0;
	}
	startLineNum = parseLineNum;
	for (;;) {	/* Until token or start of word found */
		c = pGetCharMacro();
		if (c == ' ' || c == '\t')
		  continue;		/* Quick check for white space first */
		switch (c) {
			case '#':
				while ((c = pGetChar()) != '\n' && c != PEOF) {
				}
				pUngetChar();
				continue;
			case '\\':
				if (pGetChar() == '\n') {
					startLineNum = ++parseLineNum;
					if (doPrompt)
					  putPrompt(ps2Val());
					continue;
				}
				pUngetChar();
				goto breakLoop;
			case '\n':
				++parseLineNum;
				needPrompt = doPrompt;
				RETURN(TNL);
			case PEOF:
				RETURN(TEOF);
			case '&':
				if (pGetChar() == '&')
				  RETURN(TAND);
				pUngetChar();
				RETURN(TBACKGND);
			case '|':
				if (pGetChar() == '|')
				  RETURN(TOR);
				pUngetChar();
				RETURN(TPIPE);
			case ';':
				if (pGetChar() == ';')
				  RETURN(TEND_CASE);
				pUngetChar();
				RETURN(TSEMI);
			case '(':
				RETURN(TLP);
			case ')':
				RETURN(TRP);
			default:
				goto breakLoop;
		}
	}
breakLoop:
	return readToken1(c, BASE_SYNTAX, (char *) NULL, 0);
#undef RETURN
}

/* Input any here documents.
 */
static void parseHeredoc() {
	Heredoc *heredoc;
	Node *n;

	while (heredocList) {
		heredoc = heredocList;
		heredocList = heredoc->next;
		if (needPrompt) {
			putPrompt(ps2Val());
			needPrompt = 0;
		}
		readToken1(pGetChar(), heredoc->here->type == NHERE ? SQ_SYNTAX : DQ_SYNTAX,
					heredoc->eofMark, heredoc->stripTabs);
		n = (Node *) stackAlloc(sizeof(NArg));
		n->nArg.type = NARG;
		n->nArg.next = NULL;
		n->nArg.text = wordText;
		n->nArg.backquote = backquoteList;
		heredoc->here->nHere.doc = n;
	}
}

static int readToken() {
	int t;

	t = xxReadToken();
	
	if (checkKwd) {
		/* Eat newlines */
		if (checkKwd == 2) {
			checkKwd = 0;
			while (t == TNL) {
				parseHeredoc();
				t = xxReadToken();
			}
		} else {
			checkKwd = 0;
		}
		/* CHeck for keywords */
		if (t == TWORD && ! quoteFlag) {
			register char *const *pp;

			for (pp = parseKwd; *pp; ++pp) {
				if (**pp == *wordText && equal(*pp, wordText)) {
					lastToken = t = pp - parseKwd + KWD_OFFSET;
					break;
				}
			}
		}
	}
	return t;
}

/* Read and parse a command. Returns NEOF on end of file. (NULL is a
 * valid parse tree indicating a blank line.)
 */
Node *parseCmd(int interact) {
	int t;
	extern int exitStatus;
	
	doPrompt = interact;
	if (doPrompt)
	  putPrompt(exitStatus == 0 ? ps1Val() : pseVal());
	needPrompt = 0;
	if ((t = readToken()) == TEOF)
	  return NEOF;
	if (t == TNL)
	  return NULL;
	++tokenPushback;
	return list(1);
}


#ifdef mkinit
RESET {
	tokenPushback = 0;
}
#endif


