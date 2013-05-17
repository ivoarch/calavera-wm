# Calavera-wm !!

is a minimalist stacking window manager, using prefix key style, à la ratpoison and stumpwm.

SCREENSHOTS
-------------
[![swm-2013-05-11](http://3image.com/images/2013/05/17/2013-05-17-1115521024x768scrot.th.png)](http://3image.com/images/2013/05/17/2013-05-17-1115521024x768scrot.png)

FEATURES
--------
- Floating window manager.
- Toggling windows to `center`, `maximize`, `vertical`, `horizontal`, and `full screen maximizing`.
- Good keyboard control (Emacs keybindings).
- Reasonable EWMH support (though not yet fully compliant).
- Autostart file read on startup.
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
- <kbd>C-t-Shift r</kbd> Reload calavera-wm configuration.
- <kbd>C-t-Shift q</kbd> Quit calavera-wm

### Mouse

- <kbd>ControlMask\-Button1</kbd> Move focused window.
- <kbd>ControlMask\-Button3</kbd> Resize focused window.
- <kbd>ControlMask\-Button2</kbd> Kill focused window.

Download
--------
Clone with Git by doing:

    git clone https://github.com/ivoarch/calavera-wm.git

Requirements
------------
- Libx11
- Libxinerama

Installation
------------
Calavera-wm is installed into the /usr/local namespace by default,
change the path indicated in the Makefile.

Afterwards enter the following command to build and install Calavera-wm (if
necessary as root):

    make clean install

**Note:** By default Calavera-wm will not be compiled with Xinerama support.
Uncomment the respective lines in the Makefile to change this.

Running Calavera-wm
-----------
Add the following line to your .xinitrc to start calavera-wm using startx:

    exec calavera-wm

In order to connect calavera-wm to a specific display, make sure that
the DISPLAY environment variable is set correctly, e.g.:

    DISPLAY=foo.bar:1 exec calavera-wm
    (This will start calavera-wm on display :1 of the host foo.bar.)

Configuration
-------------
The configuration of Calavera-wm is done by creating a custom conf.h
and (re)compiling the source code.

About
-----
This project is a fork of [dwm](http://dwm.suckless.org/), which was created by see [LICENSE](https://raw.github.com/ivoarch/calavera-wm/master/LICENSE) file.

Inspiration
-------------
Thanks to the authors of `DWM`, `Ratpoison`, `Stumpwm`, `Evilwm`.

Licensing
----------
This project is distributed under the MIT/X Consortium License.
