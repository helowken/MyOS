#!/bin/sh -

exec > operators.h
i=0
sed -e '/^[^#]/!d' unary_op binary_op | while read line
do
	set -$- $line
	echo "#define $1 $i"
	i=`expr $i + 1`
done
echo
echo "#define FIRST_BINARY_OP"	`sed -e '/^[^#]/!d' unary_op | wc -l`
echo '
#define OP_INT		1	/* arguments to operator are integer */
#define OP_STRING	2	/* Arguments to operator are string */
#define OP_FILE		3	/* Argument is a file name */
#define OP_LFILE	4	/* Argument is a file name of symlink? */

extern char *const unaryOp[];
extern char *const binaryOp[];
extern const char opPriority[];
extern const char opArgFlag[];'


exec > operators.c
echo '/*
 * Operators used in the expr/test command.
 */

#include "../shell.h"
#include "operators.h"

char *const unaryOp[] = {'
sed -e '/^[^#]/!d
	s/[		][	]*/ /g
	s/^[^ ][^ ]* \([^ ][^ ]*\).*/		"\1",/
	' unary_op
echo '		NULL
};

char *const binaryOp[] = {'
sed -e '/^[^#]/!d
	s/[		][	]*/ /g
	s/^[^ ][^ ]* \([^ ][^ ]*\).*/		"\1",/
	' binary_op
echo '		NULL
};

const char opPriority[] = {'
sed -e '/^[^#]/!d
	s/[		][	]*/ /g
	s/^[^ ][^ ]* [^ ][^ ]* \([^ ][^ ]*\).*/		\1,/
	' unary_op binary_op
echo '};

const char opArgFlag[] = {'
sed -e '/^[^#]/!d
	s/[		][	]*/ /g
	s/^[^ ][^ ]* [^ ][^ ]* [^ ][^ ]*$/& 0/
	s/^[^ ][^ ]* [^ ][^ ]* [^ ][^ ]* \([^ ][^ ]*\)/		\1,/
	' unary_op binary_op
echo '};'

