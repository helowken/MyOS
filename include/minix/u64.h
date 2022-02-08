#ifndef _MINIX__U64_H
#define _MINIX__U64_H

#ifndef _TYPES_H
#include "sys/types.h"
#endif

unsigned long div64u(u64_t i, unsigned j);
u64_t mul64u(unsigned long i, unsigned j);
int cmp64(u64_t i, u64_t j);
int cmp64u(u64_t i, unsigned j);
int cmp64ul(u64_t i, unsigned long j);

#endif
