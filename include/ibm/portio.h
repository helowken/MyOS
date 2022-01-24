#ifndef _PORTIO_H
#define _PORTIO_H

#include "sys/types.h"

unsigned inb(u16_t port);
unsigned inw(u16_t port);
unsigned inl(u32_t port);
void outb(u16_t port, u8_t value);
void outw(u16_t port, u16_t value);
void outl(u16_t port, u32_t value);
void disableInterrupt();
void enableInterrupt();

#endif
