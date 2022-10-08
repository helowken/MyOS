#ifndef _SETJMP_H
#define _SETJMP_H

typedef struct {
	int __flags;	
	long __mask;		/* Must have size >= sizeof(sigset_t) */
	void *__regs[16];	
} jmp_buf[1];

int __setjmp(jmp_buf _env, int _saveMask);
void longjmp(jmp_buf _env, int _val);
int sigjmp(jmp_buf _jb, int _retVal);

#define setjmp(env)	__setjmp((env), 1)

#ifdef _MINIX
#define _setjmp(env)	__setjmp((env), 0)
void _longjmp(jmp_buf _env, int _val);
#endif

#ifdef _POSIX_SOURCE
typedef jmp_buf	sigjmp_buf;
#define siglongjmp	longjmp
#define sigsetjmp(env, saveMask)	__setjmp((env), (saveMask))
#endif

#endif
