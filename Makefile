include config.mk

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
#	mkdir -p ${DESTDIR}${MANPREFIX}/man1
#	sed "s/VERSION/${VERSION}/g" < mangrove.1 > ${DESTDIR}${MANPREFIX}/man1/mangrove.1
#	ln -fs ${DESTDIR}${MANPREFIX}/man1/mangrove.1 ${DESTDIR}${MANPREFIX}/man1/mang.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/manglink
	rm -f ${DESTDIR}${PREFIX}/bin/mangd

#	${DESTDIR}${MANPREFIX}/man1/mangrove.1 \
#	${DESTDIR}${MANPREFIX}/man1/mang.1


.PHONY: all options clean install uninstall
