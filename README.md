# femto - a tiny text editor

## building


gcc femto.c -lncurses -o femto


## using


./femto text_file


## Key combinations (currently hard-coded, and oriented for colemak usage)
..* C-k    kill-line
..* C-w    kill-word
..* C-l    back-word
..* C-u    forward-word
..* C-x c  exit
..* C-x s  save-buffer
..* C-n    next-page
..* C-p    prev-page
..* C-j    insert-newline

## issues

..* does not handle unicode
..* does not handle files larger than memory


## what it looks like

![Alt text](https://github.com/ehaliewicz/femto/raw/master/screenshot.png?raw=true "")
