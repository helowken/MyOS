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

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

#endif
