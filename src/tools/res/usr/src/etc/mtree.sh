#!/bin/sh

run() 
{
	mkdir -p "$4" || exit 1
	chmod "$1" "$4" || exit 1
	chown "$2" "$4" || exit 1
	chgrp "$3" "$4" || exit 1
}

cat $1 | while read line
do 
	run $line	
	#TODO echo $line | awk '{ print "mkdir -p "$4" || exit 1; chmod "$1" "$4" || exit 1; chown "$2" "$4" || exit 1; chgrp "$3" "$4" || exit 1" }' | sh || exit
done
