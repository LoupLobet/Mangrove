include config.mk

all: clean mangrove

options:
	@echo mangrove build options:
	@echo "CFLAGS = ${CFLAGS}"
	@echo "CC     = ${CC}"

mangrove: mangrove.c
	$(CC) $(CFLAGS) -o mangrove mangrove.c

clean:
	rm -f mangrove

install: all
	# install binary
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -r mangrove ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/mangrove
	# install manuals
	# mkdir -p ${DESTDIR}${MANPREFIX}/man1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/mangrove #${DESTDIR}${PREFIX}/man1/mangrove.1

.PHONY: all options clean install uninstall
