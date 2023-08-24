#ifndef	_BITMAP_H
#define _BITMAP_H

/* Bit map operations to manipulate bits of a simple mask variable. */
#define bitSet(mask, n)		((mask) |= (1 << (n)))
#define bitUnset(mask, n)	((mask) &= ~(1 << (n)))
#define bitIsSet(mask, n)	((mask) & (1 << (n)))
#define bitEmpty(mask)		((mask) = 0)
#define bitFill(mask)		((mask) = ~0)

#endif
