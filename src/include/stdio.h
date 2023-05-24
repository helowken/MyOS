#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>

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
#ifndef NULL
#define NULL		((void *)0)
#endif
#define EOF			(-1)

#define FOPEN_MAX	20

#include <sys/dir.h>
#define FILENAME_MAX	DIRSIZ

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

int remove(const char *path);
int rename(const char *oldPath, const char *newPath);
FILE *tmpfile(void);
char *tmpnam(char *s);
int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *stream);
void setbuf(FILE *stream, char *buf);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
int fprintf(FILE * stream, const char *format, ...);
int printf(const char *format, ...);
int sprintf(char *s, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vprintf(const char *format, va_list ap);
int vsprintf(char *s, const char *format, va_list ap);
int fscanf(FILE *stream, const char *format, ...);
int scanf(const char *format, ...);
int sscanf(const char *s, const char *format, ...);
#define vfscanf	_doScan
int vfscanf(FILE *stream, const char *format, va_list ap);
int vscanf(const char *format, va_list ap);
int vsscanf(const char *s, const char *format, va_list ap);
int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int getc(FILE *stream);
int getchar(void);
char *gets(char *s);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fgetpos(FILE *stream, fpos_t *pos);
int fseek(FILE *stream, long offset, int whence);
int fsetpos(FILE *stream, fpos_t *pos);
long ftell(FILE *stream);
void rewind(FILE *stream);
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);
int __fillBuf(FILE *stream);
int __flushBuf(int c, FILE *stream);

#define getchar()	getc(stdin)
#define putchar(c)	putc(c, stdout)
#define getc(p)		(--(p)->_count >= 0 ? \
						(int) (*(p)->_ptr++) : \
						__fillBuf(p))
#define putc(c, p)	(--(p)->_count >= 0 ? \
						(int) (*(p)->_ptr++ = (c)) : \
						__flushBuf((c), (p)))

#define feof(p)		(((p)->_flags & _IOEOF) != 0)
#define ferror(p)	(((p)->_flags & _IOERR) != 0)
#define clearerr(p)	((p)->_flags &= ~(_IOERR | _IOEOF))

#ifdef _POSIX_SOURCE
int fileno(FILE *stream);
FILE *fdopen(int fd, const char *mode);
#define fileno(stream)	((stream)->_fd)
#define L_ctermid	255		/* Required by POSIX */
#define L_cuserid	255		/* Required by POSIX */
#endif

#ifdef _MINIX
int snprintf(char *s, size_t size, const char *format, ...);
int vsnprintf(char *s, size_t size, const char *format, va_list ap);
#endif

#endif
