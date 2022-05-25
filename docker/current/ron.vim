" local syntax file - set colors on a per-machine basis:
" vim: tw=0 ts=4 sw=4
" Vim color file
" Maintainer:	Ron Aaron <ron@ronware.org>
" Last Change:	2013 May 24

set background=dark
hi clear
if exists("syntax_on")
  syntax reset
endif
let g:colors_name = "ron"

hi ShowMarksHL ctermfg=cyan ctermbg=lightblue cterm=bold guifg=yellow guibg=black  gui=bold
hi cIf0			guifg=gray
hi diffOnly	guifg=red gui=bold

hi SpecialKey     term=bold cterm=bold ctermfg=4 guifg=Cyan
hi NonText        term=bold cterm=bold ctermfg=4 gui=bold guifg=yellow guibg=#303030
hi Directory      term=bold cterm=bold ctermfg=6 guifg=Cyan
hi ErrorMsg       term=standout cterm=bold ctermfg=7 ctermbg=1 guifg=Black guibg=Red
hi IncSearch      term=reverse cterm=reverse guibg=steelblue
hi Search         term=reverse ctermfg=0 ctermbg=3 gui=bold guifg=black guibg=darkgray
hi MoreMsg        term=bold cterm=bold ctermfg=2 gui=bold guifg=SeaGreen
hi ModeMsg        term=bold cterm=bold gui=bold
hi LineNr         term=underline cterm=bold ctermfg=3 guifg=darkgrey
hi CursorLineNr   term=bold cterm=bold ctermfg=3 gui=bold guifg=Yellow
hi Question       term=standout cterm=bold ctermfg=2 gui=bold guifg=Green
hi StatusLine     term=bold,reverse cterm=bold,reverse gui=bold guifg=cyan guibg=blue
hi StatusLineNC   term=reverse cterm=reverse guifg=lightblue guibg=darkblue
hi VertSplit      term=reverse cterm=reverse gui=reverse
hi Title          term=bold cterm=bold ctermfg=5 gui=bold guifg=darkgrey
hi clear Visual
hi Visual         term=reverse cterm=reverse gui=reverse
hi WarningMsg     term=standout cterm=bold ctermfg=1 guifg=Black guibg=Green
hi WildMenu       term=standout ctermfg=0 ctermbg=3 guifg=Black guibg=Yellow
hi Folded         term=standout cterm=bold ctermfg=6 ctermbg=0 guifg=Cyan guibg=gray30
hi FoldColumn     term=standout cterm=bold ctermfg=6 ctermbg=0 guifg=white guibg=gray30
hi DiffAdd        term=bold ctermbg=4 guibg=slateblue
hi DiffChange     term=bold ctermbg=5 guibg=darkgreen
hi DiffDelete     term=bold cterm=bold ctermfg=4 ctermbg=6 gui=bold guifg=Blue guibg=coral
hi DiffText       term=reverse cterm=bold ctermbg=1 gui=bold guibg=olivedrab
hi SignColumn     term=standout cterm=bold ctermfg=6 ctermbg=0 guifg=Cyan guibg=Grey
hi Conceal        ctermfg=7 ctermbg=0 guifg=LightGrey guibg=DarkGrey
hi SpellBad       term=reverse ctermbg=1 gui=undercurl guisp=Red
hi SpellCap       term=reverse ctermbg=4 gui=undercurl guisp=Blue
hi SpellRare      term=reverse ctermbg=5 gui=undercurl guisp=Magenta
hi SpellLocal     term=underline ctermbg=6 gui=undercurl guisp=Cyan
hi Pmenu          ctermfg=0 ctermbg=5 guibg=Magenta
hi PmenuSel       cterm=bold ctermfg=0 ctermbg=0 guibg=DarkGrey
hi PmenuSbar      ctermbg=7 guibg=Grey
hi PmenuThumb     ctermbg=7 guibg=White
hi TabLine        term=underline cterm=bold,underline ctermfg=7 ctermbg=0 gui=underline guibg=DarkGrey
hi TabLineSel     term=bold cterm=bold gui=bold
hi TabLineFill    term=reverse cterm=reverse gui=reverse
hi CursorColumn   term=reverse ctermbg=0 guibg=Grey40
hi CursorLine     term=underline cterm=underline guibg=Grey40
hi ColorColumn    term=reverse ctermbg=1 guibg=DarkRed
hi MatchParen     term=reverse ctermbg=6 guibg=DarkCyan
hi Comment        term=bold cterm=bold ctermfg=6 guifg=green
hi Constant       term=underline cterm=bold ctermfg=5 gui=bold guifg=cyan
hi Special        term=bold cterm=bold ctermfg=1 guifg=yellow
hi Identifier     term=underline cterm=bold ctermfg=6 guifg=cyan
hi Statement      term=bold cterm=bold ctermfg=3 guifg=lightblue
hi PreProc        term=underline cterm=bold ctermfg=4 guifg=Pink2
hi Type           term=underline cterm=bold ctermfg=2 gui=bold guifg=seagreen
hi Underlined     term=underline cterm=bold,underline ctermfg=4 gui=underline guifg=#80a0ff
hi Ignore         ctermfg=0 guifg=bg
hi Error          term=reverse cterm=bold ctermfg=7 ctermbg=1 guifg=White guibg=Red
hi Todo           term=standout ctermfg=0 ctermbg=3 guifg=Black guibg=orange
hi Label          guifg=gold2
hi Operator       guifg=orange
hi Normal         guifg=cyan guibg=black
hi Cursor         guifg=#00ff00 guibg=#60a060

