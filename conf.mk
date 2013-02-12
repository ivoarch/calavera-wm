# swm version
VERSION = `git rev-parse HEAD`

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC=/usr/include/X11
X11LIB=/usr/lib/X11

# Xinerama, comment if you don't want it
XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA

# Xft
XFTINC = -I/usr/include/freetype2
XFTLIBS = -L${X11LIB} -lXft
XFTFLAGS = -DXFT

# includes and libs
INCS = -I${X11INC} -I/usr/include/freetype2
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} -lutil -lXext -lXft -lfontconfig

# flags
CPPFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE=2 -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
#CFLAGS   = -g -std=c99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
CFLAGS   = -std=c99 -pedantic -Wall -Werror -O2 -fomit-frame-pointer -s ${INCS} ${CPPFLAGS}
LDFLAGS  = -s ${LIBS}

# compiler and linker
CC = cc
