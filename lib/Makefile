# ubus - unix bus
# See COPYING for copyright and license details.

include ../config.mk

ULIBS = libubus.a

SRC = ubus.c util.c
OBJ = ${SRC:.c=.o}

all: options ${ULIBS} 

options:
	@echo ubus build options
	@echo "  CFLAGS   = ${CFLAGS}"
	@echo "  LDFLAGS  = ${LDFLAGS}"
	@echo "  CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

libubus.a: ${OBJ}
	@echo AR libubus.a
	@ar rcs libubus.a ${OBJ}
	@chmod 755 libubus.a

libubus.so: ${OBJ}
	@echo LD libubus.so
	@${CC} -shared ${OBJ} -o libubus.so ${LIBLDFLAGS}
	@chmod 755 libubus.so

clean: 
	@echo cleaning
	@rm -f *.o *.a ${ULIBS} 

install:
	@echo installing libraries to ${DESTDIR}${PREFIX}/lib
	@for i in ${ULIBS}; \
	do \
		if [ -e $$i ]; \
		then \
			echo CP $$i; \
			cp $$i ${DESTDIR}${PREFIX}/lib/$$i; \
			chmod 755 ${DESTDIR}${PREFIX}/lib/$$i; \
		fi \
	done
	@echo installing header into ${DESTDIR}${PREFIX}/include
	@cp ubus.h ${DESTDIR}${PREFIX}/include

uninstall:
	@echo removing libraries from ${DESTDIR}${PREFIX}/lib
	@for i in ${ULIBS}; \
	do \
		if [ -e ${DESTDIR}${PREFIX}/lib/$$i ]; \
		then \
			echo RM $$i; \
			rm -f ${DESTDIR}${PREFIX}/lib/$$i; \
		fi \
	done
	@echo removing header from ${DESTDIR}${PREFIX}/include
	@rm -f ${DESTDIR}${PREFIX}/include/ubus.h

.PHONY: all options clean install uninstall

