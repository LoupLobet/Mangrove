include config.mk

all: clean options mang_open mang_new mang_del mang_link clean_object

options:
	@echo mangrove build options:
	@echo "CFLAGS = ${CFLAGS}"
	@echo "CC     = ${CC}"

.c.o :
	${CC} -c ${CFLAGS} $<

mang_open: mang_open.o util.o
	${CC} -o $@ mang_open.o util.o ${CFLAGS}

mang_new: mang_new.o util.o
	${CC} -o $@ mang_new.o util.o ${CFLAGS}

mang_del: mang_del.o util.o
	${CC} -o $@ mang_del.o util.o ${CFLAGS}

mang_link: mang_link.o util.o
	${CC} -o $@ mang_link.o util.o ${CLFAGS}

clean_object:
	rm -f mang_open.o mang_new.o mang_del.o mang_link.o util.o

clean:
	rm -f mang_open mang_new mang_del mang_link mang_open.o mang_new.o mang_del.o mang_link.o util.o

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -r mang ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/mang
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < mangrove.1 > ${DESTDIR}${MANPREFIX}/man1/mangrove.1
	ln -fs ${DESTDIR}${MANPREFIX}/man1/mangrove.1 ${DESTDIR}${MANPREFIX}/man1/mang.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/mang \
	${DESTDIR}${MANPREFIX}/man1/mangrove.1 \
	${DESTDIR}${MANPREFIX}/man1/mang.1


.PHONY: all options clean install uninstall
