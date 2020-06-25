CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=2 -pedantic -Wall -Os

all: clean mangrove

mangrove: mangrove.c
	$(CC) $(CFLAGS) -o mangrove mangrove.c

clean:
	rm -rf mangrove
