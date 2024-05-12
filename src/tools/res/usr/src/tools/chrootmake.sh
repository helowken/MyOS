#!/bin/sh

export PATH=/bin:/sbin:/usr/bin:/usr/sbin
cd /usr/src || exit 1
(cd etc && sh make)
#TODO su bin -c 'make world install' || exit 1
cd tools || exit 1
rm revision
rm -f /boot/image/*
sh make hdboot || exit 1
cp ../boot/boot /boot/boot || exit 1
mv /usr/src/commands /usr/src.commands
exit 0
