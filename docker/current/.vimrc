set nocompatible              
filetype off                  

set runtimepath^=~/.vim/plugin/cscope_maps.vim

filetype plugin indent on    

set t_Co=256
set number
set tabstop=4
set softtabstop=4
set shiftwidth=4
set linebreak
set mouse=a
set incsearch


set nocompatible

if has("syntax")
  syntax on            
endif
colorscheme ron        

filetype on
filetype plugin on

set background=dark

set autowrite        
set autoindent        
set tabstop=4        
set softtabstop=4     
set shiftwidth=4    
set cindent            
set cinoptions={0,1s,t0,n-2,p2s,(03s,=.5s,>1s,=1s,:1s     
set showmatch        
set linebreak        
set whichwrap=b,s,<,>,[,] 
set mouse=a            
set number            
set history=50        


set laststatus=2 
set ruler            

set showcmd            
set showmode        

set incsearch        
set hlsearch        


set tags=tags

"--Minix--
set tags+=~/tmp/myOSTags "myOS






