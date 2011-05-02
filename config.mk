# ubus version
VERSION = 0.5

# Customize below to fit your system

# paths
PREFIX ?= /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I. -I/usr/include -I../lib
LIBS = -L/usr/lib -lc

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\"
#CFLAGS = -g -std=gnu99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
CFLAGS = -fPIC -g -std=gnu99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
LDFLAGS = -g ${LIBS} -L../lib -lubus
#LDFLAGS = -s ${LIBS}
LIBLDFLAGS = -g ${LIBS}

# compiler and linker
CC = cc

