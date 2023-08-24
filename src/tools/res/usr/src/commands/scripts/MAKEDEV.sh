#!/bin/sh

case $1 in
-n) e=echo; shift ;;	# Just echo when -n is given.
*)  e=
esac


case $#:$1 in
1:std)		# Standard devices.	
	# $- means get options of the shell, "set -$-" means do nothing change to the shell.
	set -$- mem c0d0p0s0 \
	tty ttyc1 ttyc2 ttyc3 tty00 tty01 tty02 tty03 ttyp0 ttyp1 ttyp2 ttyp3 \
	klog random cmos
	;;
0:|1:-\?)
	cat >&2 <<EOF
Usage:  $0 [-n] key ...
Where key is one of the following:
  ram mem kmem null boot zero        # One of these makes all these memory devices
  c0d0p0s0 ...                       # Subparts c0d0p[0-3]s[0-3], ...
  console tty log                    # One of these makes all
  ttyc1 ... ttyc7                    # Virtual consoles
  tty00 ... tty03                    # Make serial lines
  ttyp0 ... ttyq0 ...                # Make tty, pty pairs
  klog                               # Make /dev/klog
  random                             # Make /dev/random, /dev/urandom
  cmos                               # Make /dev/cmos
  std                                # All standard devices
EOF
	exit 1
esac

umask 077
ex=0

for dev
do
	case $dev in	# One of the controllers? Precompute major device nr.
	c0*) maj=3	;;
	esac
	
	case $dev in
	ram|mem|kmem|null|boot|zero)
	# Memory devices.
	#
	$e mknod ram b 1 0; $e chmod 600 ram
	$e mknod mem c 1 1; $e chmod 640 mem
	$e mknod kmem c 1 2; $e chmod 640 kmem 
	$e mknod null b 1 3; $e chmod 666 null
	$e mknod boot b 1 4; $e chmod 600 boot
	$e mknod zero b 1 5; $e chmod 644 zero
	$e chgrp kmem ram mem kmem null boot zero
	;;

	c[0-3]d[0-7]p[0-3]s[0-3])
	# Disk subpartition.
	n=`expr $dev : '\\(.*\\)...'`	# Name prefix.
	d=`expr $dev : '...\\(.\\)'`	# Disk number.
	alldev=

	for p in 0 1 2 3
	do
		for s in 0 1 2 3
		do
			m=`expr 128 + $d '*' 16 + $p '*' 4 + $s`	# Minor device nr
			$e mknod ${n}${p}s${s} b $maj $m
			alldev="$alldev ${n}${p}s${s}"
		done
	done
	$e chmod 600 $alldev
	;;

	console|tty|log)
	# Console, anonymous tty, diagnostics device
	$e mknod console c 4 0
	$e chmod 600 console
	$e chgrp tty console
	$e mknod tty c 5 0
	$e chmod 666 tty
	$e mknod log c 4 15
	$e chmod 222 log
	;;

	ttyc[1-7])
	# Virtual consoles.
	m=`expr $dev : '....\\(.*\\)'`	# Minor device number.
	$e mknod $dev c 4 $m
	$e chgrp tty $dev
	$e chmod 600 $dev
	;;

	tty0[0-3])
	# Serial lines.
	n=`expr $dev : '.*\\(.\\)'`
	$e mknod $dev c 4 `expr $n + 16`
	$e chmod 666 $dev
	$e chgrp tty $dev
	;;

	tty[p-s][0-9a-f]|pty[p-s][0-9a-f])
	# Pseudo ttys.
	dev=`expr $dev : '...\\(..\\)'`
	g=`expr $dev : '\\(.\\)'`	# Which group.
	g=`echo $g | tr 'pqrs' '0123'`
	n=`expr $dev : '.\\(.\\)'`	# Which pty in the group.
	case $n in
	[a-f])	n=1`echo $n | tr 'abcdef' '012345'`
	esac

	$e mknod tty$dev c 4 `expr $g '*' 16 + $n + 128`
	$e mknod pty$dev c 4 `expr $g '*' 16 + $n + 192`
	$e chgrp tty tty$dev pty$dev
	$e chmod 666 tty$dev pty$dev
	;;


	random|urandom)
	# random data generator.
	$e mknod random c 16 0; $e chmod 644 random
	$e mknod urandom c 16 0; $e chmod 644 urandom
	$e chgrp operator random urandom
	;;

	cmos)
	# cmos device (set/get system time).
	$e mknod cmos c 17 0
	$e chmod 600 cmos
	;;

	klog)
	# logging device.
	$e mknod klog c 15 0
	$e chmod 600 klog
	;;

	*)
	echo "$0: don't know about $dev" >&2
	ex=1
	esac
done

exit $ex


