#!/bin/bash

# Wallpaper
bgs -s ~/Fotos/Wallpapers/Gnome3.jpg
#xsetroot -solid '#2e2e2e'

# Hide cursor
#unclutter -idle 3 -jitter 2 -root &

# Java recognizes
wmname LG3D &

# external disk tray
ejectsy &

# battery tray
cbatticon -i symbolic -c 5 -u 5 &

# Start Dropbox
dropboxd &

# Network Manager
nm-applet &

# Wicd
#wicd-client --tray &

# Browser
conkeror &
