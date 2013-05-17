# swm version
VERSION = `git rev-parse HEAD`

# paths
PREFIX = /usr/local

X11INC=/usr/include/X11
X11LIB=/usr/lib/X11

# OPTIONAL XINERAMA

# Xinerama, uncomment if you want it
#XINERAMALIBS = -lXinerama
#XINERAMAFLAGS = -DXINERAMA

# includes and libs
INCS = -I${X11INC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} 

# flags
CPPFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE=2 -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS} 
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
#CFLAGS = -g -std=c99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
#CFLAGS = -std=c99 -pedantic -Wall -Werror -O2 -fomit-frame-pointer -s ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}

# compiler and linker
CC = cc

SRC = swm.c
OBJ = ${SRC:.c=.o}

all: options swm

options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@${CC} -c ${CFLAGS} $<

${OBJ}: conf.h 

swm: ${OBJ}
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@rm -f swm ${OBJ}

install: all
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f swm ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/swm
	@cp swm.desktop /usr/share/xsessions

uninstall:
	@rm -f ${DESTDIR}${PREFIX}/bin/swm

.PHONY: all options clean dist install uninstall
