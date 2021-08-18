#ifndef _DEBUG_H
#define _DEBUG_H

#ifndef DEBUG
#define DEBUG	0
#endif

#if !DEBUG
#define NDEBUG	1
#define debug(expr)	((void) 0)
#else
#define debug(expr)	expr
#endif

#endif
