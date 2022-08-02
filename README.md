Please install bochs first.
	% cd bochs
	% make


=======================================================================
When you want to editing source files, open a new terminal and then 
	% ./edit
You can use vim to edit all the files.
When editing in vim, commands below are use to create tags and make.
	:copen			# open a make window
	:make			# run make
	:cclose			# close the make window 

	:cd /usr/src/myOS	# change to source root dir 
	:!./run_ctag	# create tags


=======================================================================
When you want to run the OS, open a new terminal and then
	% ./run

After you are in the container, compile and install all
	% make			# compile all 
	% cd boot		
	% make install	# install all to the device

If you want to debug with bochs or run simply:
	% make run

If you want to debug with GDB
	% make gdb
and open a new terminal
	% ./attach
then go to the corresponding module dir, E.g kernel
	% cd kernel
	% ./run_gdb


=======================================================================
If you want to read the asm code, first go into the container 
through the "run" or "attach" shell, then go to the module dir, 
E.g kernel:
	% cd kernel
	% make disasm



