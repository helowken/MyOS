#include "code.h"
#include "stdarg.h"
#include "stddef.h"
#include "limits.h"

#define isdigit(c)	((unsigned) ((c) - '0') < (unsigned) 10)
#define count_kputc(c)	do { charCount++; kputc(c); } while(0)

extern void kputc(int c);

int printf(const char *fmt, ...) {
	int c, charCount = 0;
	enum { LEFT, RIGHT } adjust;
	enum { LONG, INT } intSize;
	int fill, width, max, len, base;
	static char X2C_tab[] = "0123456789ABCDEF";
	static char x2c_tab[] = "0123456789abcdef";

	char *x2c, *p;
	long i;
	unsigned long u;
	char temp[8 * sizeof(long) / 3 + 2];

	va_list argp;

	va_start(argp, fmt);

	while ((c = *fmt++) != 0) {
		if (c != '%') {
			count_kputc(c);
			continue;
		}
		
		c = *fmt++;

		adjust = RIGHT;
		if (c == '-') {
			adjust = LEFT;
			c = *fmt++;
		}

		fill = ' ';
		if (c == '0') {
			fill = '0';
			c = *fmt++;
		}
		
		width = 0;
		if (c == '*') {
			width = va_arg(argp, int);
			c = *fmt++;
		} else if (isdigit(c)) {
			do {
				width = width * 10 + (c - '0');
			} while (isdigit(c = *fmt++));
		}

		max = INT_MAX;
		if (c == '.') {
			if ((c = *fmt++) == '*') {
				max = va_arg(argp, int);
				c = *fmt++;
			} else if (isdigit(c)) {
				max = 0;
				do {
					max = max * 10 + (c - '0');
				} while (isdigit(c = *fmt++));
			}
		}
		
		x2c = x2c_tab;
		i = 0;
		base = 10;
		intSize = INT;
		if (c == 'l' || c == 'L') {
			intSize = LONG;
			c = *fmt++;
		}
		if (c == 0) 
		  break;

		switch (c) {
			case 'd':
				i = intSize == LONG ? va_arg(argp, long)
								: va_arg(argp, int);
				u = i < 0 ? -i : i;
				goto int2ascii;
			case 'o':
				base = 010;
				goto getInt;
			case 'p':
				if (sizeof(char *) > sizeof(int)) 
				  intSize = LONG;
			case 'X':
				x2c = X2C_tab;
			case 'x':
				base = 0x10;
				goto getInt;
			case 'u':
getInt:
				u = intSize == LONG ? va_arg(argp, unsigned long)
								: va_arg(argp, unsigned int);

int2ascii:
				p = temp + sizeof(temp) - 1;
				*p = 0;
				do {
					*--p = x2c[(ptrdiff_t) (u % base)];
				} while ((u /= base) > 0);
				goto string_length;

			case 'c':
				p = temp;
				*p = va_arg(argp, int);
				len = 1;
				goto string_print;

			case '%':
				p = temp;
				*p = '%';
				len = 1;
				goto string_print;

			case 's':
				p = va_arg(argp, char *);

string_length:
				for (len = 0; p[len] != 0 && len < max; ++len) {}

string_print:
				width -= len;
				if (i < 0)
				  --width;
				if (fill == '0' && i < 0)
				  count_kputc('-');
				if (adjust == RIGHT) {
					while (width > 0) {
						count_kputc(fill);
						--width;
					}
				}
				if (fill == ' ' && i < 0)
				  count_kputc('-');
				while (len > 0) {
					count_kputc((unsigned char) *p++);
					--len;
				}
				while (width > 0) {
					count_kputc(fill);
					--width;
				}
				break;

			default:
				count_kputc('%');
				count_kputc(c);
		}
	}

	kputc(0);
	va_end(argp);
	return charCount;
}

