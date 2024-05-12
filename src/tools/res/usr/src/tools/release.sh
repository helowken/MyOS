#!/bin/sh

usr_kb()
{
	# $4 is the used value
	echo $4 
}

du_size()
{
	echo $1
}

sum()
{
	total=0
	for num in "$@"
	do
		total=$(expr total + $num)
	done
	echo $total
}

secs=`expr 32 '*' 64`

COPYITEMS="usr/bin bin usr/lib"
RELEASEDIR=/usr/r
IMAGE=cdfdimage
ROOTIMAGE=rootimage
CDFILES=/usr/tmp/cdreleasefiles
version_pretty=`cat version`
version=`echo ${version_pretty} | tr . _`
ISO=minix${version}_`date +%Y%m%d-%H%M%S`
RAM=/dev/ram
BS=4096

HDEMU=0

ISO=${ISO}.iso
ISOGZ=${ISO}.gz
echo "Making $ISOGZ"

USRMB=100

USRBLOCKS="`expr $USRMB \* 1024 \* 1024 / $BS`"
USRSECTS="`expr $USRMB \* 1024 \* 2`"
ROOTKB=1400
ROOTBLOCKS="`expr $ROOTKB \* 1024 / $BS`"
ROOTSECTS="`expr $ROOTKB \* 2`"

echo "Warning: I'm going to mkfs $RAM! It has to be at least $ROOTKB KB."
echo ""
echo "Temporary (sub)partition to use to make the /usr FS image? "
echo "I need $USRMB MB. It will be mkfsed!"
echo -n "Device: /dev/"
dev=c0d0p1s0	#read dev || exit 1
TMPDISK=/dev/$dev

if [ -b $TMPDISK ]
then :
else
	echo "$TMPDISK is not a block device.."
	exit 1
fi

echo "Temporary (sub)partition to use for /tmp? "
echo "It will be mkfsed!"
echo -n "Device: /dev/"
dev=c0d0p1s1	#read dev || exit 1
TMPDISK2=/dev/$dev

if [ -b $TMPDISK2 ]
then :
else
	echo "$TMPDISK2 is not a block device.."
	exit 1
fi

umount $TMPDISK
umount $TMPDISK2
umount $RAM

echo " * Cleanup old files"
rm -rf $RELEASEDIR $ISO $IMAGE $ROOTIMAGE $ISOGZ $CDFILES
mkdir -p $CDFILES || exit
mkdir -p $RELEASEDIR
echo " * Zeroing $RAM"
dd if=/dev/zero of=$RAM bs=$BS count=$ROOTBLOCKS
mkfs -B $BS -b $ROOTBLOCKS $RAM || exit
mkfs $TMPDISK2 || exit
echo " * mounting $RAM as $RELEASEDIR"
mount $RAM $RELEASEDIR || exit

mkdir -m 755 $RELEASEDIR/usr
mkdir -m 1777 $RELEASEDIR/tmp
mount $TMPDISK2 $RELEASEDIR/tmp

echo " * Zeroing $TMPDISK"
#TODO dd if=/dev/zero of=$TMPDISK bs=$BS count=$USRBLOCKS
mkfs -B $BS -b $USRBLOCKS $TMPDISK || exit
echo " * Mounting $TMPDISK as $RELEASEDIR/usr"
mount $TMPDISK $RELEASEDIR/usr || exit
mkdir -p $RELEASEDIR/tmp
mkdir -p $RELEASEDIR/usr/tmp

echo " * Transfering $COPYITEMS to $RELEASEDIR"
for item in $COPYITEMS
do
	if [ "$item" = "bin" ]
	then
		( cd / && cp -R $item $RELEASEDIR ) || exit 1
	else
		( cd / && cp -R $item $RELEASEDIR/usr ) || exit 1
	fi
done

# Make sure compilers and libraries are bin-owned
chown -R bin $RELEASEDIR/usr/lib
chmod -R u+w $RELEASEDIR/usr/lib

# Copy resources to release dir
cp -R /sbin $RELEASEDIR
cp -R /usr/src /usr/sbin $RELEASEDIR/usr

# Make sure the CD knows it's a CD
date >$RELEASEDIR/CD
echo " * Chroot build"
chroot $RELEASEDIR "/bin/sh -x /usr/src/tools/chrootmake.sh" || exit 1
echo " * Chroot build done"
# The build process leaves some file in src as root.
chown -R bin $RELEASEDIR/usr/src*

echo "Temporary filesystems still mounted. Make changes, or press RETURN"
echo -n "to continue making the image.."
read xyzzy

echo $version_pretty >$RELEASEDIR/etc/version
echo " * Counting files"
usr_kb $(df $TMPDISK | tail -n1) >$RELEASEDIR/.usrkb
du -s $RELEASEDIR/usr/src.* | while read size
do
	sum `du_size $size` >$RELEASEDIR/.extrasrckb
done
( for d in $RELEASEDIR/usr/src.*; do find $d; done ) | wc -l >$RELEASEDIR/.extrasrcfiles
find $RELEASEDIR/usr | wc -l >$RELEASEDIR/.usrfiles
find $RELEASEDIR -xdev | wc -l >$RELEASEDIR/.rootfiles
umount $TMPDISK || exit
umount $TMPDISK2 || exit
umount $RAM || exit
sh make clean
sh make image || exit 1
cp image image_big
sh make clean
dd if=$RAM of=$ROOTIMAGE bs=$BS count=$ROOTBLOCKS
# Prepare image for cdfdboot
cp image_big image
sh mkboot cdfdboot
cp $IMAGE $CDFILES/bootflop.img
#TODO cp release/cd/* $CDFILES

h_opt=
bootimage=$IMAGE
writeisofs -l MINIX -b $bootimage $h_opt $CDFILES $ISO || exit 1


