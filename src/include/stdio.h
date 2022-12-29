#ifndef _STDIO_H
#define _STDIO_H

typedef struct __iobuf {
	int	_count;
	int	_fd;
	int	_flags;
	int	_bufSize;
	unsigned char *_buf;
	unsigned char *_ptr;
} FILE;

#define _IOFBF		0x000
#define _IOREAD		0x001
#define _IOWRITE	0x002
#define _IONBF		0x004
#define _IOMYBUF	0x008
#define _IOEOF		0x010
#define _IOERR		0x020
#define _IOLBF		0x040
#define _IOREADING	0x080
#define _IOWRITING	0x100
#define _IOAPPEND	0x200
#define _IOFIFO		0x400

/* The following definitions are also in <unistd.h>. They should not
 * conflict.
 */
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define stdin		(&__stdin)
#define stdout		(&__stdout)
#define stderr		(&__stderr)

#define BUFSIZ		1024
#define NULL		((void *)0)
#define EOF			(-1)

#define FOPEN_MAX	20

#include "sys/dir.h"
#define FILENAME_MAX	DIR_SIZE

#define TMP_MAX		999
#define L_tmpnam	(sizeof("/tmp") + FILENAME_MAX)
#define __STDIO_VA_LIST__	void *

typedef long int		fpos_t;

#ifndef _SIZE_T
#define _SIZE_T	
typedef unsigned int	size_t;		/* Type returned by sizeof */
#endif

extern FILE *__iotab[FOPEN_MAX];
extern FILE __stdin, __stdout, __stderr;

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
int fflush(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int ferror(FILE *stream);
int printf(const char *format, ...);

#define getchar()	getc(stdin)
#define putchar(c)	putc(c, stdout)
#define getc(p)		(--(p)->_count >= 0 ? \
						(int) (*(p)->_ptr++) : \
						_fillBuf(p))
#define putc(c, p)	(--(p)->_count >= 0 ? \
						(int) (*(p)->_ptr++ = (c)) : \
						__flushBuf((c), (p)))

#define feof(p)		(((p)->_flags & _IOEOF) != 0)
#define ferror(p)	(((p)->_flags & _IOERR) != 0)
#define clearerr(p)	((p)->flags &= ~(_IOERR | _IOEOF))

#ifdef _POSIX_SOURCE
int fileno(FILE *stream);
FILE *fdopen(int fildes, const char *types);
#define fileno(stream)	((stream)->_fd)
#define L_ctermid	255		/* Required by POSIX */
#define L_cuserid	255		/* Required by POSIX */
#endif

#ifdef _MINIX
int snprintf(char *str, size_t size, const char *format, ...);
#endif

#endif
