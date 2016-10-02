femto - a tiny text editor

building
----

gcc femto.c -lncurses -o femto


using
----

./femto text_file

type anything you want (Ctrl-X s/c for save and quit).


issues
----

- does not handle unicode
- does not handle resizing your terminal
- does not handle files larger than memory


what it looks like
----
![Alt text](https://github.com/ehaliewicz/femto/raw/master/screenshot.png?raw=true "")
