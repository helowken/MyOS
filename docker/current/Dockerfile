FROM helowken/myos-base-dev:latest
ENV HOME=/usr/src
ENV BOCHS_NORMAL=$HOME/bochs/build/normal
ENV BOCHS_GDB=$HOME/bochs/build/gdb
WORKDIR $HOME
COPY .vimrc $HOME
COPY cscope_maps.vim $HOME/.vim/plugin/
