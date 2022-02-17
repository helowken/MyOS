#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "error_functions.h"
#include "../include/ibm/partition.h"

typedef enum { false, true } bool;

#define min(m,n) ((m) < (n) ? (m) : (n))
#define max(m,n) ((m) > (n) ? (m) : (n))

#define EXTERN	extern

typedef unsigned long	Ino_t;		/* i-node numer */
typedef unsigned long	Block_t;	/* block number */
typedef unsigned long	Off_t;		/* Offset within a file */
typedef unsigned short	Mode_t;		/* File type and permissions bits */
typedef unsigned long	Zone_t;		/* Zone number */
typedef unsigned short	Bitchunk_t;	/* Collection of bits in a bitmap */
typedef short			Dev_t;		/* Holds (major|minor) device pair */
typedef short			Nlink_t;	/* Number of links to a file */
typedef long			Time_t;

#endif
