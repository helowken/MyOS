BOCHS=bochs-20211104

# unzip Bochs
user$ tar xvf $BOCHS.tar.gz

# edit .bochsrc, comment two lines below:
cpu: model=...
cpu: cpuid...
# save .bochsrc and go on.

# copy config file
user$ cp my.conf.linux $BOCHS
user$ cd $BOCHS

# clean build and reset config
user$ rm -rf ../build
user$ make clean

# config
user$ ./my.conf.linux

# compile and install
user$ make
user$ make install
