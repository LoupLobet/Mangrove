include config.mk

all: options mangrove

options:
	@echo mangrove build options:
	@echo "CFLAGS = ${CFLAGS}"
	@echo "CC     = ${CC}"

mangrove: mangrove.c
	${CC} ${CFLAGS} -o mangrove mangrove.c

clean:
	rm -f mangrove

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -r mangrove ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/mangrove
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < mangrove.1 > ${DESTDIR}${MANPREFIX}/man1/mangrove.1
	ln -s ${DESTDIR}${MANPREFIX}/man1/mangrove.1 ${DESTDIR}${MANPREFIX}/man1/mang.1 

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/mangrove ${DESTDIR}${PREFIX}/man1/mangrove.1 ${DESTDIR}${PREFIX}/man1/mang.1

.PHONY: all options clean install uninstall
