# mangrove version
VERSION = 0.1.3

# compiler
CC = cc

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags

# If you are using BSD comment next line and `#include bsd/unistd.h` in mangrove.c
LINUX = -lbsd

CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=2 -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS} ${LINUX}

