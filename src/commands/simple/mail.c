#include "sys/types.h"
#include "sys/stat.h"
#include "errno.h"
#undef EOF			/* Temporary hack */
#include "signal.h"
#include "pwd.h"
#include "time.h"
#include "setjmp.h"
#include "string.h"
#include "stdlib.h"
#include "fcntl.h"
#include "unistd.h"
#include "sys/wait.h"
#include "stdio.h"

#ifdef DEBUG
#define D(Q)	(Q)
#else
#define D(Q)	
#endif

#define SHELL		"/bin/sh"

#define DROP_NAME	"/usr/spool/mail/%s"
#define LOCK_NAME	"/usr/spool/mail/%s.lock"
#define LOCK_WAIT	5	/* Seconds to wait after collision */
#define LOCK_TRIES	4	/* Maximum number of collisions */

#define MBOX		"mbox"

#define HELP_FILE	"/usr/lib/mail.help"
#define	PROMPT		"? "
#define PATH_LEN	80
#define MAX_RCPT	100	/* Maximum number of recipients */
#define LINE_LEN	512

#define UNREAD		1	/* 'Not read yet' status */
#define DELETED		2	/* 'Deleted' status */
#define READ		3	/* 'Has been read' status */

typedef struct Letter {
	struct Letter *prev, *next;		/* Linked letter list */
	int status;			/* Letter status */
	off_t location;		/* Location within mailbox file */
} Letter;

static Letter *firstLet, *lastLet;
static int useMailer = 1;	/* Use MAILER to deliver (if any) */
static int printMode = 0;	/* Print-and-exit mode */
static int quitMode = 0;	/* Take interrupts */
static int reverseMode = 0;	/* Print mailbox in reverse order */
static int useDrop = 1;		/* Read the maildrop (no -f given) */
static int verbose = 0;		/* Pass "-v" flag on to mailer */
static int needUpdate = 0;	/* Need to update mailbox */
static int msgStatus = 0;	/* Return the mail status */
static int distList = 0;	/* Include distribution list */
static char mailbox[PATH_LEN];	/* User's mailbox/maildrop */
static char tempName[PATH_LEN] = "/tmp/mailXXXXXX";	/* Temporary file */
static char *subject = NULL;
static FILE *boxFp = NULL;	/* Mailbox file */
static jmp_buf printJump;	/* For quitting out of letters */
static unsigned oldMask;	/* Saved umask() */

extern int optind;
extern char *optarg;

static char *baseName(char *name) {
	char *p;

	if ((p = rindex(name, '/')) == NULL)
	  return name;
	return p + 1;
}

static void usage() {
	fprintf(stderr, "usage: mail [-epqr] [-f file]\n");
	fprintf(stderr, "       mail [-dtv] [-s subject] user [...]\n");
}

static char *whoami() {
	struct passwd *pw;

	if ((pw = getpwuid(getuid())) != NULL)
	  return pw->pw_name;

	return "nobody";
}

static int fileSize(char *name) {
	struct stat sb;

	if (stat(name, &sb) == -1)
	  sb.st_size = 0L;

	return sb.st_size ? 1 : 0;
}

static FILE *makeRewindable() {
/* 'stdin' isn't rewindable. Make a temp file that is.
 * Note that if one wanted to catch SIGINT and write a '~/dead.letter'
 * for interactive mails, this might be the place to do it (though the
 * case where a MAILER is being used would also need to be handled).
 */
	FILE *tempFp;	/* Temp file used for copy */
	int c;			/* Character being copied */
	int state;		/* ".\n" detection state */

	if ((tempFp = fopen(tempName, "w")) == NULL) {
		fprintf(stderr, "mail: can't create temporary file\n");
		return NULL;
	}

	/* Here we copy until we reach the end of the letter (end of file or
	 * a line containing only a '.'), painstakingly avoiding setting a
	 * line length limit. */
	state = '\n';
	while ((c = getc(stdin))) {
		switch (state) {
			case '\n':
				if (c == '.') {
					state = '.';
				} else {
					if (c != '\n')
					  state = '\0';
					putc(c, tempFp);
				}
				break;
			case '.':
				if (c == '\n')	/* \n.\n -> done */
				  goto done;
				state = '\0';
				putc('.', tempFp);
				putc(c, tempFp);
				break;
			default:
				state = (c == '\n' ? '\n' : '\0');
				putc(c, tempFp);
		}
	}
done:
	if (ferror(tempFp) || fclose(tempFp)) {
		fprintf(stderr, "mail: couldn't copy letter to temporary file\n");
		return NULL;
	}
	tempFp = freopen(tempName, "r", stdin);
	unlink(tempName);	/* Unlink name; file lingers on in limbo */
	return tempFp;
}

static int copy(FILE *fromFp, FILE *toFp) {
	int c;					/* Character being copied */
	int state;				/* ".\n" and postmark detection state */
	int blankLine = 0;		/* Was most recent line completely blank? */
	static char postmark[] = "From ";
	char *p, *q;

	/* Here we copy until we reach the end of the letter (end of file or
	 * a line containing only a '.'). Postmarks (lines beginning with
	 * "From ") are copied with a ">" prepended. Here we also complicate
	 * things by not setting a line limit. */
	state = '\n';
	p = postmark;
	while ((c = getc(fromFp)) != EOF) {
		switch (state) {
			case '\n':
				if (c == '.') {	/* '.' at beginning of line */
					state = '.';
				} else if (*p == c) {	/* Start of postmark */
					++p;
					state = 'P';
				} else {	/* Anything else */
					if (c == '\n') {
						blankLine = 1;
					} else {
						state = '\0';
						blankLine = 0;
					}
					putc(c, toFp);
				}
				break;
			case '.':
				if (c == '\n')
				  goto done;
				state = '\0';
				putc('.', toFp);
				putc(c, toFp);
				break;
			case 'P':
				if (*p == c) {
					if (*++p == '\0') {	/* Successfully reached end */
						p = postmark;
						putc('>', toFp);
						fputs(postmark, toFp);
						state = '\0';
						break;
					}
					break;	/* Not there yet */
				}
				state = (c == '\n' ? '\n' : '\0');
				for (q = postmark; q < p; ++q) {
					putc(*q, toFp);
				}
				putc(c, toFp);
				blankLine = 0;
				p = postmark;
				break;
			default:
				state = (c == '\n' ? '\n' : '\0');
				putc(c, toFp);
		}
	}
	if (state != '\n')
	  putc('\n', toFp);
done:
	if (! blankLine)
	  putc('\n', toFp);
	if (ferror(toFp))
	  return -1;
	return 0;
}

static int deliver(int count, char *vec[]) {
	int i, j;
	int errs = 0;		/* Count of errors */
	int dropFd;			/* File descriptor for user's drop */
	int created = 0;	/* True if we created the maildrop */
	FILE *mailFp;		/* Fp for mail */
	struct stat stb;	/* For checking drop modes, owners */
	/* Saveing signal state */
	void (*sigint)(int); 
	void (*sighup)(int);
	void (*sigquit)(int);
	time_t now;			/* For datestamping the postmark */
	char sender[32];	/* Sender's login name */
	char lockName[PATH_LEN];	/* MailDrop lock */
	int lockTries;		/* Tries when box is locked */
	struct passwd *pw;	/* Sender and recipent */
	int toConsole;		/* Deliver to console if everything fails */

	if (count > MAX_RCPT) {
		fprintf(stderr, "mail: too many recipients\n");
		return -1;
	}
	if ((pw = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "mail: unknown sender\n");
		return -1;
	}
	strcpy(sender, pw->pw_name);

	/* If we need to rewind stdin and it isn't rewindable, make a copy */
	if (isatty(0) || (count > 1 && lseek(0, 0L, SEEK_SET) == (off_t) -1)) 
	  mailFp = makeRewindable();
	else
	  mailFp = stdin;

	/* Shut off signals during the delivery */
	sigint = signal(SIGINT, SIG_IGN);
	sighup = signal(SIGHUP, SIG_IGN);
	sigquit = signal(SIGQUIT, SIG_IGN);

	for (i = 0; i < count; ++i) {
		if (count > 1)
		  rewind(mailFp);

		D(printf("deliver to %s\n", vec[i]));

		if ((pw = getpwnam(vec[i])) == NULL) {
			fprintf(stderr, "mail: user %s not known\n", vec[i]);
			++errs;
			continue;
		}
		sprintf(mailbox, DROP_NAME, pw->pw_name);
		sprintf(lockName, LOCK_NAME, pw->pw_name);

		D(printf("maildrop='%s', lock='%s'\n", mailbox, lockName));

		/* Lock the maildrop while we're messing with it. Races are
		 * possible (though not very likely) when we have to create
		 * the maildrop, but not otherwise. If the box is already
		 * locked, wait awhile and try again.
		 */
		lockTries = created = toConsole = 0;
tryLock:
		if (link(mailbox, lockName) != 0) {
			if (ENOENT == errno) {	/* User doesn't have a drop yet */
				dropFd = creat(mailbox, 0600);
				if (dropFd < 0 && errno == ENOENT) {
					/* Probably missing spool dir; to console. */
					boxFp = fopen("/dev/console", "w");
					if (boxFp != NULL) {
						toConsole = 1;
						goto noBox;
					}
				}
				if (dropFd < 0) {
					fprintf(stderr, 
						"mail: couldn't create a maildrop for user %s\n", 
						vec[i]);
					++errs;
					continue;
				}
				++created;
				goto tryLock;
			} else {	/* Somebody else has it locked, it seems - wait */
				if (lockTries >= LOCK_TRIES) {
					fprintf(stderr, 
						"mail: couldn't lock maildrop for user %s\n",
						vec[i]);
					++errs;
					continue;
				}
				sleep(LOCK_WAIT);
				goto tryLock;
			}
		}
		if (created) {
			chown(mailbox, pw->pw_uid, pw->pw_gid);
			boxFp = fdopen(dropFd, "a");
		} else {
			boxFp = fopen(mailbox, "a");
		}

		if (boxFp == NULL || stat(mailbox, &stb) < 0) {
			fprintf(stderr, 
				"mail: serious maildrop problems for %s\n", vec[i]);
			unlink(lockName);
			++errs;
			continue;
		}
		if (stb.st_uid != pw->pw_uid || ! S_ISREG(stb.st_mode)) {
			fprintf(stderr,
				"mail: mailbox for user %s is illegal\n", vec[i]);
			unlink(lockName);
			++errs;
			continue;
		}
noBox:
		if (toConsole) {
			fprintf(boxFp,
				"-------------\n| Mail from %s to %s\n-------------\n",
				sender, vec[i]);
		} else {
			time(&now);
			fprintf(boxFp, "From %s %24.24s\n", sender, ctime(&now));
		}

		/* Add the To: header line */
		fprintf(boxFp, "To: %s\n", vec[i]);

		if (distList) {
			fprintf(boxFp, "Dist: ");
			for (j = 0; j < count; ++j) {
				if (getpwnam(vec[j]) != NULL && j != i)
				  fprintf(boxFp, "%s ", vec[j]);
			}
			fprintf(boxFp, "\n");
		}

		/* Add the Subject: header line */
		if (subject != NULL)
		  fprintf(boxFp, "Subject: %s\n", subject);

		fprintf(boxFp, "\n");

		if (copy(mailFp, boxFp) < 0 || fclose(boxFp) != 0) {
			fprintf(stderr, 
				"mail: error delivering to user %s", vec[i]);
			perror(" ");
			++errs;
		}
		unlink(lockName);
	}

	fclose(mailFp);

	/* Put signals back the way they were */
	signal(SIGINT, sigint);
	signal(SIGHUP, sighup);
	signal(SIGQUIT, sigquit);

	return errs == 0 ? 0 : -1;
}

static void readBox() {
	char lineBuf[512];
	Letter *let;
	off_t current;

	firstLet = lastLet = NULL;

	if (access(mailbox, R_OK | X_OK) < 0 || (boxFp = fopen(mailbox, "r")) == NULL) {
		if (useDrop && errno == ENOENT)
		  return;
		fprintf(stderr, "Can't access mailbox ");
		perror(mailbox);
		exit(EXIT_FAILURE);
	}
	current = 0L;
	while (1) {
		if (fgets(lineBuf, sizeof(lineBuf), boxFp) == NULL)
		  break;

		if (strncmp(lineBuf, "From ", (size_t) 5) == 0) {
			if ((let = (Letter *) malloc(sizeof(Letter))) == NULL) {
				fprintf(stderr, "Out of memory.\n");
				exit(EXIT_FAILURE);
			}
			if (lastLet == NULL) {
				firstLet = let;
				let->prev = NULL;
			} else {
				let->prev = lastLet;
				lastLet->next = let;
			}
			lastLet = let;
			let->next = NULL;

			let->status = UNREAD;
			let->location = current;
			D(printf("letter at %ld\n", current));
		}
		current += strlen(lineBuf);
	}
}

static void printLet(Letter *let, FILE *toFp) {
	off_t current, limit;
	int c;

	current = let->location;
	fseek(boxFp, current, SEEK_SET);
	limit = (let->next != NULL ? let->next->location : -1);

	while (current != limit && (c = getc(boxFp)) != EOF) {
		putc(c, toFp);
		++current;
	}
}

static void printAll() {
	Letter *let;

	let = reverseMode ? firstLet : lastLet;

	if (let == NULL) {
		printf("No mail.\n");
		return;
	} 
	while (let != NULL) {
		printLet(let, stdout);
		let = reverseMode ? let->next : let->prev;
	}
}

static void onInt(int dummy) {
	longjmp(printJump, 1);
}

static void saveLet(Letter *let, char *saveFile) {
	int waitStat, pid;
	FILE *saveFp;

	if ((pid = fork()) < 0) {
		perror("mail: couldn't fork");
		return;
	} else if (pid != 0) {	/* Parent */
		wait(&waitStat);	
		return;
	}

	/* Child */
	setgid(getgid());
	setuid(getuid());
	if ((saveFp = fopen(saveFile, "a")) == NULL) {
		perror(saveFile);
		exit(EXIT_FAILURE);
	}
	printLet(let, saveFp);
	if (ferror(saveFp) != 0 || fclose(saveFp) != 0) {
		fprintf(stderr, "savefile write error: ");
		perror(saveFile);
	}
	exit(EXIT_SUCCESS);
}

static void doShell(char *command) {
	int waitStat, pid;
	char *shell;

	if ((shell = getenv("SHELL")) == NULL)
	  shell = SHELL;

	if ((pid = fork()) < 0) {
		perror("mail: couldn't fork");
		return;
	} else if (pid != 0) {	/* Parent */
		wait(&waitStat);
		return;
	}

	/* Child */
	setgid(getgid());
	setuid(getuid());
	umask(oldMask);
	
	execl(shell, shell, "-c", command, (char *) NULL);
	fprintf(stderr, "can't exec shell\n");
	exit(127);
}

static void doHelp() {
	FILE *fp;
	char buffer[80];

	if ((fp = fopen(HELP_FILE, "r")) == NULL) {
		fprintf(stdout, "can't open helpfile %s\n", HELP_FILE);
	} else {
		while (fgets(buffer, 80, fp)) {
			fputs(buffer, stdout);
		}
	}
}

static void interact() {
	char lineBuf[512];		/* User input line */
	Letter *let, *next;		/* Current and next letter */
	int interrupted = 0;	/* SIGINT hit during letter print */
	int needPrint = 1;		/* Need to print this letter */
	char *saveFile;			/* FileName to save into */

	if (firstLet == NULL) {
		printf("No mail.\n");
		return;
	}
	let = reverseMode ? firstLet : lastLet;

	while (1) {
		next = reverseMode ? let->next : let->prev;
		if (next == NULL)
		  next = let;

		if (! quitMode) {
			interrupted = setjmp(printJump);
			signal(SIGINT, onInt);
		}
		if (! interrupted && needPrint) {
			if (let->status != DELETED)
			  let->status = READ;
			printLet(let, stdout);
		}
		if (interrupted)
		  putchar('\n');
		needPrint = 0;
		fputs(PROMPT, stdout);
		fflush(stdout);

		if (fgets(lineBuf, sizeof(lineBuf), stdin) == NULL)
		  break;

		if (! quitMode)
		  signal(SIGINT, SIG_IGN);

		switch (lineBuf[0]) {
			case '\n':
				let = next;
				needPrint = 1;
				continue;
			case 'd':
				let->status = DELETED;
				if (let != next)
				  needPrint = 1;
				needUpdate = 1;
				let = next;
				continue;
			case 'p':
				needPrint = 1;
				continue;
			case '-':
				next = reverseMode ? let->prev : let->next;
				if (next == NULL)
				  next = let;
				let = next;
				needPrint = 1;
				continue;
			case 's':
				for (saveFile = strtok(lineBuf + 1, " \t\n"); 
						saveFile != NULL;
						saveFile = strtok((char *) NULL, " \t\n")) {
					saveLet(let, saveFile);
				}
				continue;
			case '!':
				doShell(lineBuf + 1);
				continue;
			case '*':
				doHelp();
				continue;
			case 'q':
				return;
			case 'x':
				exit(EXIT_SUCCESS);
			default:
				fprintf(stderr, "Illegal command\n");
				continue;
		}
	}
}

static void updateBox() {
	FILE *tempFp;				/* Fp for tempFile */
	char lockName[PATH_LEN];	/* Maildrop lock */
	int lockTries = 0;			/* Tries when box is locked */
	Letter *let;				/* Current letter */
	int c;

	sprintf(lockName, LOCK_NAME, whoami());

	if ((tempFp = fopen(tempName, "w")) == NULL) {
		perror("mail: can't create temporary file");
		return;
	}
	for (let = firstLet; let != NULL; let = let->next) {
		if (let->status != DELETED) {
			printLet(let, tempFp);
			D(printf("printed letter at %ld\n", let->location));
		}
	}

	if (ferror(tempFp) || (tempFp = freopen(tempName, "r", tempFp)) == NULL) {
		perror("mail: temporary file write error");
		unlink(tempName);
		return;
	}

	/* Shut off signals during the update */
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	if (useDrop) {
		while (link(mailbox, lockName) != 0) {
			if (++lockTries >= LOCK_TRIES) {
				fprintf(stderr, "mail: couldn't lock maildrop for update\n");
				return;
			}
			sleep(LOCK_WAIT);
		}
	}

	if ((boxFp = freopen(mailbox, "w", boxFp)) == NULL) {
		perror("mail: couldn't reopen maildrop");
		fprintf(stderr, "mail may have been lost; look in %s\n", tempName);
		if (useDrop)
		  unlink(lockName);
		return;
	}
	unlink(tempName);

	while ((c = getc(tempFp)) != EOF) {
		putc(c, boxFp);
	}
	fclose(boxFp);

	if (useDrop)
	  unlink(lockName);
}

int main(int argc, char **argv) {
	int c;

	if ('l' == (baseName(argv[0]))[0])	/* 'lmail' link? */
	  useMailer = 0;	/* Yes, let's deliver it */

	mktemp(tempName);	/* Name the temp file */
	
	oldMask = umask(022);	/* Change umask for security */

	while ((c = getopt(argc, argv, "epqrf:tdvs:")) != EOF) {
		switch (c) {
			case 'e': ++msgStatus; break;
			case 't': ++distList; break;
			case 'p': ++printMode; break;
			case 'q': ++quitMode; break;
			case 'r': ++reverseMode; break;
			case 'f': 
				setuid(getuid());	/* Won't need to lock */ 
				useDrop = 0; 
				strncpy(mailbox, optarg, (size_t) (PATH_LEN - 1));
				break;
			case 'd': useMailer = 0; break;
			case 'v': ++verbose; break;
			case 's': subject = optarg; break;
			default: 
				usage();
				exit(EXIT_FAILURE);

		}
	}

	if (optind < argc) {
		if (deliver(argc - optind, argv + optind) < 0)
		  exit(EXIT_FAILURE);
		else 
		  exit(EXIT_SUCCESS);
	}
	if (useDrop)
	  sprintf(mailbox, DROP_NAME, whoami());

	D(printf("mailbox=%s\n", mailbox));

	if (msgStatus) {
		if (fileSize(mailbox))
		  exit(EXIT_SUCCESS);
		else
		  exit(EXIT_FAILURE);
	}

	readBox();

	if (printMode)
	  printAll();
	else
	  interact();

	if (needUpdate)
	  updateBox();

	return EXIT_SUCCESS;
}


