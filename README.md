femto - a modal editor
RSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRSTRST
building
-----

gcc femto.c -lncurses -o femto


using
----

mode 1:

- type anything you want (except Ctrl-x)


mode 2:

- type Ctrl-x, then type s to save, or c to quit.
- anything else quits mode 2




issues
-----

- does not handle unicode
- does not handle resizing your terminal

what it looks like
----
![Alt text](https://github.com/ehaliewicz/femto/raw/master/screenshot.png?raw=true "")
