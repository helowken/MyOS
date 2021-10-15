#ifndef _PORTIO_H
#define _PORTIO_H

#include "sys/types.h"

unsigned inb(U16_t port);
unsigned inw(U16_t port);
unsigned inl(U32_t port);
void outb(U16_t port, U8_t value);
void outw(U16_t port, U16_t value);
void outl(U16_t port, U32_t value);
void disableInterrupt();
void enableInterrupt();

#endif
