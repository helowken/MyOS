#ifndef _TERMIOS_H
#define	_TERMIOS_H

typedef unsigned short tcflag_t;	
typedef unsigned char cc_t;		
typedef unsigned int speed_t;		

#define NCCS	20	/* Size of c_ctrl_char array, some extra space for extensions. */

/* Private terminal control structure. */
struct termios {
	tcflag_t c_iflag;	/* input modes */
	tcflag_t c_oflag;	/* output modes */
	tcflag_t c_cflag;	/* control modes */
	tcflag_t c_lflag;	/* local modes */
	speed_t c_ispeed;	/* input speed */
	speed_t c_ospeed;	/* output speed */
	cc_t c_cc[NCCS];	/* control characters */
};

/* Values for termios c_iflag bit map. */
#define BRKINT		0x0001	/* Signal interrupt on break */
#define ICRNL		0x0002	/* Map CR to NL on input */
#define IGNBRK		0x0004	/* Ignore break */
#define IGNCR		0x0008	/* Ignore CR */
#define IGNPAR		0x0010	/* Ignore characters with parity errors */
#define INLCR		0x0020	/* Map NL to CR on input */
#define INPCK		0x0040	/* Enable input parity check */
#define ISTRIP		0x0080	/* Mask off 8th bit */
#define IXOFF		0x0100	/* Enable start/stop input control */
#define IXON		0x0200	/* Enable start/stop output control */
#define PARMRK		0x0400	/* Mark parity errors in the input queue */

/* Values for termios c_oflag bit map. */
#define OPOST		0x0001	/* Perform output processing */

/* Values for termios c_cflag bit map. */
#define CLOCAL		0x0001	/* Ignoremodem status lines */
#define CREAD		0x0002	/* Enable receiver */
#define CSIZE		0x000C	/* Number of bits per character */
#define		CS5	0x0000		/* If CSIZE is CS5, characters are 5 bits */
#define		CS6 0x0004		/* If CSIZE is CS6, characters are 6 bits */
#define		CS7 0x0008		/* If CSIZE is CS7, characters are 7 bits */
#define		CS8 0x000C		/* If CSIZE is CS8, characters are 8 bits */
#define CSTOPB		0x0010	/* Send 2 stop bits if set, else 1 */
#define HUPCL		0x0020	/* Hang up on last close */
#define PARENB		0x0040	/* Enable parity on output */
#define PARODD		0x0080	/* Use odd parity if set, else even */

/* Values for termios c_lflag bit map. */
#define ECHO		0x0001	/* Enable echoing of input characters */
#define ECHOE		0x0002	/* Echo ERASE as backspace */
#define ECHOK		0x0004	/* Echo KILL */
#define ECHONL		0x0008	/* Echo NL */
#define ICANON		0x0010	/* Canonical input (erase and kill enabled) */
#define IEXTEN		0x0020	/* Enable extended functions */
#define ISIG		0x0040	/* Enable signals */
#define NOFLSH		0x0080	/* Disable flush after interrupt or quit */
#define TOSTOP		0x0100	/* Send SIGTTOU (job control, not implemented) */

/* Indices into c_cc array. Default values in parentheses. */
#define VEOF		0		/* cc_c[VEOF] = EOF char (^D) */
#define VEOL		1		/* cc_c[VEOL] = EOL char (undef) */
#define VERASE		2		/* cc_c[VERASE] = ERASE char (^H) */
#define VINTR		3		/* cc_c[VINTR] = INTR char (DEL) */
#define VKILL		4		/* cc_c[VKILL] = KILL char (^U) */
#define VMIN		5		/* cc_c[VMIN] = MIN value for timer */
#define VQUIT		6		/* cc_c[VQUIT] = QUIT char (^\) */
#define VTIME		7		/* cc_c[VTIME] = TIME value for timer */
#define VSUSP		8		/* cc_c[VSUSP] = SUSP (^Z, ignored) */
#define VSTART		9		/* cc_c[VSTART] = START char (^S) */
#define VSTOP		10		/* cc_c[VSTOP] = STOP char (^Q) */

#define _POSIX_VDISABLE		(cc_t) 0xFF		/* You can't even generate this
											 * character with 'normal' keyboards.
											 * But some language specific keyboards
											 * can generate 0xFF. It seems that all
											 * 256 are used, so cc_t should be a 
											 * short...
											 */

/* Values for the baud rate settings. */
#define B0			0x0000	/* Hang up the line */
#define B50			0x1000	/* 50 baud */
#define B75			0x2000	/* 75 baud */
#define B110		0x3000	/* 110 baud */
#define B134		0x4000	/* 134.5 baud */
#define B150		0x5000	/* 150 baud */
#define B200		0x6000	/* 200 baud */
#define B300		0x7000	/* 300 baud */
#define B600		0x8000	/* 600 baud */
#define B1200		0x9000	/* 1200 baud */
#define B1800		0xA000	/* 1800 baud */
#define B2400		0xB000	/* 2400 baud */
#define B4800		0xC000	/* 4800 baud */
#define B9600		0xD000	/* 9600 baud */
#define B19200		0xE000	/* 19200 baud */
#define B38400		0xF000	/* 38400 baud */

/* Optional actions for tcsetattr(). */
#define TCSANOW			1	/* Changes take effect immediately */
#define TCSADRAIN		2	/* Changes take effect after output is done */
#define TCSAFLUSH		3	/* Wait for output to finish and flush input */

/* Queue selector values for tcflush() */
#define TCIFLUSH		1	/* Flush accumulated input data */
#define TCOFLUSH		2	/* Flush accumulated output data */
#define TCIOFLUSH		3	/* Flush accumulated input and output data */

/* Action values for tcflow(). */
#define TCOOFF			1	/* Suspend output */
#define TCOON			2	/* Restart suspended output */
#define TCIOFF			3	/* Transmit a STOP character on the line */
#define TCION			4	/* Transmit a START character on the line */

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int opt_actions, const struct termios *termios_p);


#ifdef _MINIX
/* Here are the local extensions to the POSIX standard for Minix. Posix
 * conforming programs are not able to access these, and therefore they are
 * only defined when a Minix program is compiled.
 */

/* Extensions to the termios c_iflag bit map. */
#define IXANY		0x0800	/* Allow any key to continue output */

/* Extensions to the termios c_oflag bit map. They are only active iff
 * OPOST is enabled. */
#define ONLCR		0x0002	/* Map NL to CR-NL on output */
#define XTABS		0x0004	/* Expand tabs to spaces */
#define ONOEOT		0x0008	/* Discard EOT's (^D) on output */

/* Extensions to the c_cc array. */
#define VREPRINT		11	/* cc_c[VREPRINT] (^R) */
#define VLNEXT			12	/* cc_c[VLNEXT]	(^V) */
#define VDISCARD		13	/* cc_c[VDISCARD] (^O) */

/* There are the default settings used by the kernel and by 'stty sane' */
#define TCTRL_DEF	(CREAD | CS8 | HUPCL)
#define TINPUT_DEF	(BRKINT | ICRNL | IXON | IXANY)
#define TOUTPUT_DEF	(OPOST | ONLCR)
#define TLOCAL_DEF	(ISIG | IEXTEN | ICANON | ECHO | ECHOE)
#define TSPEED_DEF	B9600

/* See "man ascii" for more. */
#define TEOF_DEF	'\4'	/* ^D = C('D') = 0x1F & 0x44 = 0x4 = '\4' */
#define TEOL_DEF	_POSIX_VDISABLE
#define TERASE_DEF	'\10'	/* ^H = C('H') = 0x1F & 0x48 = 0x8 = '\10' */
#define TINTR_DEF	'\3'	/* ^C */
#define TKILL_DEF	'\25'	/* ^U = C('U') = 0x1F & 0x55 = 0x15 = '\25' */
#define TMIN_DEF	1
#define TQUIT_DEF	'\34'	/* ^\ = C('\\') = 0x1F & 0x5C = 0x1C = '\34' */
#define TSTART_DEF	'\21'	/* ^Q */
#define TSTOP_DEF	'\23'	/* ^S */
#define TSUSP_DEF	'\32'	/* ^Z */
#define TTIME_DEF	0
#define TREPRINT_DEF	'\22'	/* ^R */
#define TLNEXT_DEF	'\26'	/* ^V */
#define TDISCARD_DEF	'\17'	/* ^O */

/* Window size. This information is stored in the TTY driver but not used.
 * This can be used for screen based applications in a window environment.
 * The ioctls TIOCGWINSZ and TIOCSWINSZ can be used to get and set this
 * information.
 */
typedef struct {
	unsigned short ws_row;		/* Rows, in characters */
	unsigned short ws_col;		/* Columns, in characters */
	unsigned short ws_xpixel;	/* Horizontal size, pixels */
	unsigned short ws_ypixel;	/* Vertical size, pixels */
} WinSize;
#endif	/* _MINIX */

#endif
