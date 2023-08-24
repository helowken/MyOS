#!/bin/sh

# Shell script used to test MINIX.

PATH=:/bin:/usr/bin
export PATH

echo -n "Shell test  1 "
rm -rf DIR_SH1
mkdir DIR_SH1
cd DIR_SH1

f=../test1
if test -r $f; then : ; else echo sh1 connot read $f; exit 1; fi

# Initial setup
echo "abcdefghijklmnopqrstuvwxyz" >alpha
echo "ABCDEFGHIJKLMNOPQRSTUVWXYZ" >ALPHA
echo "0123456789" >num
echo "!@#$%^&*()_+=-{}[]:;<>?/.," >special
cp /etc/rc rc
cp /etc/passwd passwd
cat alpha ALPHA num rc passwd special >tmp
cat tmp tmp tmp >f1


# Test cp
mkdir foo
cp /etc/rc /etc/passwd foo
if cmp -s foo/rc /etc/rc ; then : ; else echo Error on cp test 1; fi
if cmp -s foo/passwd /etc/passwd ; then : ; else echo Error on cp test 2; fi
rm -rf foo

# Test cat
cat num num num num num >y
wc -c y >x1
echo "     55 y" >x2
if cmp -s x1 x2; then : ; else echo Error on cat test 1; fi
cat <y >z
if cmp -s y z; then : ; else echo Error on cat test 2; fi

# Test basename
if test `basename /usr/ast/foo.c .c` != 'foo'
then echo Error on basename test 1
fi

if test `basename a/b/c/d` != 'd'; then Error on basename test 2; fi

# Test cdiff, sed, and patch
# TODO

# Test comm, grep -v
# TODO
