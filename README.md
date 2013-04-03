# SWM

is a minimalist stacking window manager, using prefix key style, à la ratpoison and stumpwm.

SCREENSHOTS
--------------------
[![gimp](http://ompldr.org/taHhreg)](http://ompldr.org/vaHhreg/2013-03-30-113939_1024x768_scrot.png)
[![conkeror](http://ompldr.org/taHhsMA)](http://ompldr.org/vaHhsMA/2013-03-30-114110_1024x768_scrot.png)
[![icons](http://ompldr.org/taHhwaw)](http://ompldr.org/vaHhwaw/2013-03-30-190326_1024x768_scrot.png)

FEATURES
--------
- Floating window manager.
- Toggling windows to `center`, `maximize` and `full screen maximizing`.
- Good keyboard control (Emacs keybindings).
- Reasonable EWMH support (though not yet fully compliant).
- Autostart file read on startup.
- Support for xft fonts.
- System tray.
- Xinerama support.
- Few dependencies :)

Commands
-----------------

### keyboard shortcuts

- <kbd>C-t c</kbd> Start `Urxvt`.
- <kbd>C-t F2</kbd> Start app launcher.
- <kbd>C-t e</kbd> Start `Emacs`.
- <kbd>C-t w</kbd> Start `Conkeror`.
- <kbd>C-t l</kbd> Locks the screen with `Xlock`.
- <kbd>C-t f</kbd> Toggles fullscreen.
- <kbd>C-t m</kbd> Maximise focused window.
- <kbd>C-t .</kbd> Center focused window.
- <kbd>C-t n</kbd> Focus next window.
- <kbd>C-t p</kbd> Focus previous window.
- <kbd>C-t k</kbd> Close focused window.
- <kbd>C-t-Shift 1,2,3</kbd> Move focused window to another workspace.
- <kbd>C-t 1,2,3</kbd> Switch to workspace number 1|2|...
- <kbd>C-t-Shift r</kbd> Reload swm configuration.
- <kbd>C-t-Shift q</kbd> Quit swm.

### Mouse

- <kbd>ControlMask\-Button1</kbd> Move focused window.
- <kbd>ControlMask\-Button2</kbd> Resize focused window.

About
-------
This project is a fork of [dwm](http://dwm.suckless.org/), which was created by see [LICENSE](https://raw.github.com/ivoarch/swm/master/LICENSE) file.

Download
--------
Clone with Git by doing:

    git clone https://github.com/ivoarch/swm.git

Requirements
------------
In order to build swm you need the header files.

- Libx11
- Libxft
- Libxinerama

Installation
------------
Edit conf.mk to match your local setup (swm is installed into
the /usr/local namespace by default).

Afterwards enter the following command to build and install swm (if
necessary as root):

    make clean install

If you are going to use the default bluegray color scheme it is highly
recommended to also install the bluegray files shipped in the dextra package.


Running swm
-----------
Add the following line to your .xinitrc to start swm using startx:

    exec swm

In order to connect swm to a specific display, make sure that
the DISPLAY environment variable is set correctly, e.g.:

    DISPLAY=foo.bar:1 exec swm
(This will start swm on display :1 of the host foo.bar.)

Configuration
-------------
The configuration of swm is done by creating a custom conf.h
and (re)compiling the source code.
