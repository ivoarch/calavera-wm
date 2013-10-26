# Calavera-wm ☠

is a minimalist stacking window manager, using prefix key style, à la ratpoison and stumpwm.

FEATURES
---------

> TODO

- Floating window manager.
- Toggling windows to `center`, `maximize`, `vertical`, `horizontal`, and `full screen maximizing`.
- Good keyboard control (Emacs keybindings).
- Autostart file read on startup.
- Multimedia keys (working)
- Does not have Xinerama support.
- Few dependencies :)

COMMANDS
-----------------

### keyboard shortcuts

<kbd>Control-t</kbd> is the default prefix keys.

- <kbd>c</kbd> Start `Urxvt`.
- <kbd>a</kbd> Start app launcher.
- <kbd>e</kbd> RunOrRaise `Emacs`.
- <kbd>w</kbd> RunOrRaise `Conkeror`.
- <kbd>l</kbd> Locks the screen with `Xlock`.
- <kbd>b</kbd> Banish the mouse cursor.
- <kbd>f</kbd> Toggles fullscreen.
- <kbd>m</kbd> Maximise focused window.
- <kbd>v</kbd> Vertical maximise.
- <kbd>h</kbd> Horizontal maximise.
- <kbd>.</kbd> Center focused window.
- <kbd>n</kbd> Focus next window.
- <kbd>p</kbd> Focus previous window.
- <kbd>k</kbd> Close focused window.
- <kbd>Shift 1,2,3</kbd> Move focused window to another workspace.
- <kbd>1,2,3</kbd> Switch to workspace number 1|2|...
- <kbd>Shift r</kbd> Reload calavera-wm configuration.
- <kbd>Shift q</kbd> Quit calavera-wm

### mouse

<kbd>Control</kbd> is the default prefix key for the mouse

- <kbd>Left button</kbd> Move the window.
- <kbd>Right button</kbd> Resize the window.
- <kbd>Middle button</kbd> Close the window.

Download
--------
Clone with Git by doing:

    git clone https://github.com/ivoarch/calavera-wm.git

Requirements
------------
- Libx11

Installation
------------
Calavera-wm is installed into the /usr/local namespace by default,
change the path indicated in the Makefile.

Afterwards enter the following command to build and install Calavera-wm (if
necessary as root):

    make clean install

Running Calavera-wm
-----------
Add the following line to your .xinitrc to start calavera-wm using startx:

    exec calavera-wm

Configuration
-------------
The configuration of Calavera-wm is done by creating a custom conf.h
and (re)compiling the source code.

Inspiration
-------------
Thanks to the authors of `DWM`, `Ratpoison`, `Stumpwm`, `Evilwm`.

About/Licensing
----------------
This project is a fork of [dwm](http://dwm.suckless.org/), which was created by see [LICENSE](https://raw.github.com/ivoarch/calavera-wm/master/LICENSE) file.
