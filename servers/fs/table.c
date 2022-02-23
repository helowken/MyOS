#define _TABLE

#include "fs.h"
#include "minix/callnr.h"
#include "minix/com.h"
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "super.h"
#include "inode.h"
#include "lock.h"

int (*callVec[])() = {

};
