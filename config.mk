# mangrove version
VERSION = 0.1

# compiler
CC = cc

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
LIBS = -lbsd
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=2 -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Wno-depreciated-declarations -Os ${CPPFLAGS} ${LIBS}

