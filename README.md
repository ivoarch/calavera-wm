# SWM

swm is a minimalist stacking window manager, using prefix key style, à la ratpoison and stumpwm.

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

In order to display status info in the bar, you can do something
like this in your .xinitrc:

    while true ; do
        xsetroot -name "$(acpi -b | awk 'sub(/,/,"") {print $3, $4}')"
        sleep 1m
    done &
    exec swm

Configuration
-------------
The configuration of swm is done by creating a custom conf.h
and (re)compiling the source code.
