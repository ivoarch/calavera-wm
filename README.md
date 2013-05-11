# SWM

is a minimalist stacking window manager, using prefix key style, à la ratpoison and stumpwm.

SCREENSHOTS
--------------------
[![swm-2013-05-11](http://ompldr.org/taWRnbA)](http://ompldr.org/vaWRnbA/Screenshot%20from%202013-05-11%2021:14:26.png)

FEATURES
--------
- Floating window manager.
- Toggling windows to `center`, `maximize`, `verticalmax`, `horizontalmax`, and `full screen maximizing`.
- Good keyboard control (Emacs keybindings).
- Reasonable EWMH support (though not yet fully compliant).
- Autostart file read on startup.
- System tray.
- (Optional) Xft fonts support.
- (Optional) Xinerama support.
- Few dependencies :)

Commands
-----------------

### keyboard shortcuts

- <kbd>C-t c</kbd> Start `Urxvt`.
- <kbd>C-t a</kbd> Start app launcher.
- <kbd>C-t e</kbd> Start `Emacs`.
- <kbd>C-t w</kbd> Start `Conkeror`.
- <kbd>C-t l</kbd> Locks the screen with `Xlock`.
- <kbd>C-t f</kbd> Toggles fullscreen.
- <kbd>C-t m</kbd> Maximise focused window.
- <kbd>C-t v</kbd> Vertical maximise.
- <kbd>C-t h</kbd> Horizontal maximise.
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
- <kbd>ControlMask\-Button3</kbd> Resize focused window.
- <kbd>ControlMask\-Button2</kbd> Kill focused window.

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
Swm is installed into the /usr/local namespace by default,
change the path indicated in the Makefile.

Afterwards enter the following command to build and install swm (if
necessary as root):

    make clean install

**Note:** By default Swm will not be compiled with Xinerama and XFT support.
Uncomment the respective lines in the Makefile to change this.

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
