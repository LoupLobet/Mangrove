# mangrove version
VERSION = 0.1.3

# compiler
# gcc clang and tcc have been tested and seem to work for compilation
CC = cc

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# If you are using BSD comment next line 
# LINUX = -lbsd

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=2 -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS} ${LINUX}

