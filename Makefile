# Calavera-wm version
#VERSION = `git rev-parse HEAD`
VERSION = 1.0

# paths
PREFIX = /usr/local

X11INC=/usr/include/X11
X11LIB=/usr/lib/X11

# includes and libs
INCS = -I${X11INC}
LIBS = -L${X11LIB} -lX11

# flags
CPPFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE=2 -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}

# compiler and linker
CC = cc

SRC = calavera-wm.c
OBJ = ${SRC:.c=.o}

all: options calavera-wm

options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@${CC} -c ${CFLAGS} $<

${OBJ}: conf.h

calavera-wm: ${OBJ}
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@rm -f calavera-wm ${OBJ}

install: all
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f calavera-wm ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/calavera-wm
	@cp calavera-wm.desktop /usr/share/xsessions

uninstall:
	@rm -f ${DESTDIR}${PREFIX}/bin/calavera-wm
	@rm -f /usr/share/xsessions/calavera-wm.desktop

.PHONY: all options clean dist install uninstall
