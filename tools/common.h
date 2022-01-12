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

typedef enum { false, true } bool;

#define min(m,n) ((m) < (n) ? (m) : (n))
#define max(m,n) ((m) > (n) ? (m) : (n))

#endif
