#!/bin/sh
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
cd /usr/src || exit 1
(cd etc && sh make.sh)
#TODO su bin -c 'make world install' || exit 1
cd tools || exit 1
rm revision
rm /boot/image/*
echo make hdboot || exit 1
