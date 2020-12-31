VERSION = 1.0
CC = cc
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/man
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=2 -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS} ${LINUX}

all: clean options manglink mangd

options:
	@echo mangrove build options:
	@echo "CFLAGS = ${CFLAGS}"
	@echo "CC     = ${CC}"

.c.o :
	${CC} -c ${CFLAGS} $<

mang_link: manglink.o
	${CC} -o $@ mang_link.o ${CLFAGS}
mangd: mangd.o
	${CC} -lkvm -o $@ mangd.o ${CLFAGS}

clean_object:
	rm -f mang*.o
clean:
	rm -f manglink mangd manglink.o mangd.o

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -r manglink mangd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/manglink ${DESTDIR}${PREFIX}/bin/mangd
	sed "s/VERSION/${VERSION}/g" < manglink.1 > ${DESTDIR}${MANPREFIX}/man1/manglink.1
	sed "s/VERSION/${VERSION}/g" < mangd.8 > ${DESTDIR}${MANPREFIX}/man8/mangd.8
	sed "s/VERSION/${VERSION}/g" < mangd.conf.5 > ${DESTDIR}${MANPREFIX}/man5/mangd.conf.5

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/manglink
	rm -f ${DESTDIR}${PREFIX}/bin/mangd
	rm -f ${DESTDIR}${MANPREFIX}/man1/manglink.1 
	rm -f ${DESTDIR}${MANPREFIX}/man8/mangd.8 
	rm -f ${DESTDIR}${MANPREFIX}/man5/mangd.conf.5 

.PHONY: all options clean install uninstall
