# MyOS User Guide

## Description

MyOS is derived from Minix3 and now is built in a docker image. 
The docker image contains a Linux development env which integrates 
some necessary software, such as vim, gcc, gdb, bochs, etc.

Note:
* All the assembly are rewritten by Linux AS.
* All the source files are compiled by gcc and ELF is the only binary format.
* Most of the c code are the same as in minix3 book, but some calculation are different since ELF is used.

## Getting Started

### Dependencies

* Intel CPU >= 386
* Docker env.

If you are using a Windows system, VirtualBox + Ubuntu/mint is suggested.

### Installing and executing

* The "run" shell starts a container with full functionality.
```
./run
```
* Install bochs.
```
% cd ~/bochs
% make
```
* Compile all.
```
cd ~/myOS
make clean
make
```
* Install boot and images to device.
```
% cd ~/myOS/boot
% make install
```
* Run with bochs debugger.
```
% cd ~/myOS/boot
% make run
```
* Run with GDB.
```
% cd ~/myOS/boot
% make gdb
```
* Open a new terminal and use "attach" shell to start a container.
```
./attach
```
* Go into a module and start debugging.
```
% cd ~/myOS/kernel	# debug kernel
% ./run_gdb

% cd ~/myOS/servers/fs	# debug FS
% ./run_gdb
```

### Editing source files

* The "edit" shell starts a container with only editing and make functions.
```
./edit
% vim kernel/main.c

# When editing source files, some commands are useful.
:copen		# open a make window
:make		# run make
:cclose		# close the make window
:tabe main.c	# open main.c in a new tab
:gt/gT		# forward/backward navigation
```


