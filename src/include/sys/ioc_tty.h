#ifndef _S_I_TTY_H
#define	_S_I_TTY_H

#include "minix/ioctl.h"

/* Terminal ioctls. */
#define TC_GET			_IOR('T',  8, struct termios)	/* tcgetattr */
#define TC_SET_NOW		_IOW('T',  9, struct termios)	/* tcsetattr, TCSANOW */
#define TC_SET_DRAIN	_IOW('T', 10, struct termios)	/* tcsetattr, TCSADRAIN */
#define TC_SET_FLUSH	_IOW('T', 11, struct termios)	/* tcsetattr, TCSAFLUSH */
#define TC_SEND_BREAK	_IOW('T', 12, int)	/* tcsendbreak */
#define TC_DRAIN		_IO('T', 13)		/* tcdrain */
#define TC_FLOW			_IOW('T', 14, int)	/* tcflow */
#define TC_FLUSH		_IOW('T', 15, int)	/* tcflush */
#define TIOC_GET_WINSZ	_IOR('T', 16, WinSize)
#define TIOC_SET_WINSZ	_IOW('T', 17, WinSize)
#define TIOC_GET_PGRP	_IOR('T', 18, int)
#define TIOC_SET_PGRP	_IOW('T', 19, int)
#define TIOC_SET_FONT	_IOW('T', 20, u8_t [8192])

/* Keyboard ioctls. */
#define KIOC_SET_MAP	_IOW('k', 3, keymap_t)

#endif
