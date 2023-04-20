#undef assert

#ifdef NDEBUG
/* Debugging disabled */
#define assert(expr)	((void) 0)
#else
#define __str(x)	# x
#define __xstr(x)	__str(x)

void __bad_assertion(const char *_mess);
#define assert(expr)	((expr)? (void) 0 : \
			__bad_assertion("Assertion \"" #expr \
				"\" failed, file " __xstr(__FILE__) \
				", line " __xstr(__LINE__) "\n"))
#endif
