# ubus - unix bus
# See COPYING for copyright and license details.

include ../config.mk

BINS = ubus ubus-connect ubus-signal ubus-invoke

all: options ${BINS} 

options:
	@echo ubus cli build options
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${BINS}: % : %.o
	@echo CC -o $@
	@${CC} -o $@ $< ${LDFLAGS}

clean: 
	@echo cleaning
	@rm -f *.o *.a ${BINS}

install:
	@echo installing cli executables to ${DESTDIR}${PREFIX}/bin
	@for i in ${BINS}; \
	do \
		if [ -e $$i ]; \
		then \
			echo CP $$i; \
			cp $$i ${DESTDIR}${PREFIX}/bin/$$i; \
			chmod 755 ${DESTDIR}${PREFIX}/bin/$$i; \
		fi \
	done

uninstall:
	@echo removing cli executables from ${DESTDIR}${PREFIX}/bin
	@for i in ${BINS}; \
	do \
		if [ -e ${DESTDIR}${PREFIX}/bin/$$i ]; \
		then \
			echo RM $$i; \
			rm -f ${DESTDIR}${PREFIX}/bin/$$i; \
		fi \
	done

.PHONY: all options clean install uninstall

