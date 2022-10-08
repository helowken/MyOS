#ifndef _STDIO_H
#define _STDIO_H

typedef struct __iobuf {
	int		_count;
	int		_fd;
	int		_flags;
	int		_bufSize;
	unsigned char	*_buf;
	unsigned char	*_ptr;
} FILE;

#define BUFSIZ		1024

#ifndef _SIZE_T
#define _SIZE_T	
typedef unsigned int	size_t;		/* Type returned by sizeof */
#endif

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int ferror(FILE *stream);
int printf(const char *format, ...);

int snprintf(char *str, size_t size, const char *format, ...);

#endif
