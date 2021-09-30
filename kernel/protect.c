#include "kernel.h"
#include "proc.h"
#include "protect.h"



PUBLIC SegDesc gdt[GDT_SIZE];		/* Used in mpx.s */
