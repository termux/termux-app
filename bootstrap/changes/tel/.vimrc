python3 from powerline.vim import setup as powerline_setup
python3 powerline_setup()
python3 del powerline_setup
set noshowmode
call plug#begin('~/.vim/plugged')
Plug 'https://github.com/junegunn/vim-github-dashboard.git'
Plug 'scrooloose/nerdtree', { 'on':  'NERDTreeToggle' }
Plug 'junegunn/limelight.vim'
Plug 'vim-scripts/AutoComplPop'
call plug#end()
" Only do this part when compiled with support for autocommands
if has("autocmd")
augroup redhat
autocmd!
"       " When editing a file, always jump to the last cursor position
autocmd BufReadPost *
\ if line("'\"") > 0 && line ("'\"") <= line("$") |
\   exe "normal! g'\"" |
\ endif
augroup END
endif
