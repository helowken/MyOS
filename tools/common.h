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

/* Mapping Linux types to Minix types. */
typedef	int16_t			Uid_t;		/* User id */
typedef int8_t			Gid_t;		/* Group id */
typedef uint32_t		Ino_t;		/* i-node numer */
typedef uint32_t		Block_t;	/* block number */
typedef uint32_t		Off_t;		/* Offset within a file */
typedef uint16_t		Mode_t;		/* File type and permissions bits */
typedef uint32_t		Zone_t;		/* Zone number */
typedef uint16_t		Bitchunk_t;	/* Collection of bits in a bitmap */
typedef int16_t			Dev_t;		/* Holds (major|minor) device pair */
typedef int16_t			Nlink_t;	/* Number of links to a file */
typedef int32_t			Time_t;

#endif
