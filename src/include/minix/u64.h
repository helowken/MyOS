#ifndef _MINIX__U64_H
#define _MINIX__U64_H

#ifndef _TYPES_H
#include <sys/types.h>
#endif

u64_t add64u(u64_t i, unsigned j);
u64_t add64ul(u64_t i, unsigned long j);
u64_t mul64u(unsigned long i, unsigned j);
u64_t cvu64(unsigned i);
u64_t cvul64(unsigned long i);
unsigned long div64u(u64_t i, unsigned j);
unsigned rem64u(u64_t i, unsigned j);
unsigned cv64u(u64_t i);
unsigned long cv64ul(u64_t i);
int cmp64(u64_t i, u64_t j);
int cmp64u(u64_t i, unsigned j);
int cmp64ul(u64_t i, unsigned long j);
unsigned long ex64lo(u64_t i);
unsigned long ex64hi(u64_t i);
u64_t make64(unsigned long lo, unsigned long hi);

#endif
